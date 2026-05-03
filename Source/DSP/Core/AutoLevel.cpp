#include "AutoLevel.h"

void AutoLevel::prepare(double sampleRate)
{
    fs = sampleRate;

    // 最大10秒分のヒストリーバッファを事前確保 (192kHz環境でも約200万サンプル。メモリ負荷は極小)
    const int maxSamples = static_cast<int>(std::ceil(192000.0 * 10.0));
    historyBuffer.assign(maxSamples, 0.0f);
    writeIndex = 0;

    // ゲイン適用時にポップノイズを防ぐため、50msで滑らかにフェードさせる
    outputGain.reset(sampleRate, 0.05);
    outputGain.setCurrentAndTargetValue(1.0f); // 初期ゲインは0dB(1.0)
}

void AutoLevel::pushSample(float input)
{
    // 波形をリングバッファに上書き録音していく
    historyBuffer[writeIndex] = input;
    writeIndex++;
    if (writeIndex >= static_cast<int>(historyBuffer.size()))
        writeIndex = 0;
}

float AutoLevel::calculateRMS(float seconds) const
{
    int numSamples = static_cast<int>(fs * seconds);
    int bufferSize = static_cast<int>(historyBuffer.size());

    if (numSamples > bufferSize) numSamples = bufferSize;
    if (numSamples <= 0) return 0.0f;

    double sumSquares = 0.0;
    int readIdx = writeIndex - 1;

    // 指定された秒数だけ過去に遡り、二乗和を計算する
    for (int i = 0; i < numSamples; ++i)
    {
        if (readIdx < 0) readIdx += bufferSize;
        float val = historyBuffer[readIdx];
        sumSquares += static_cast<double>(val * val);
        readIdx--;
    }

    return static_cast<float>(std::sqrt(sumSquares / numSamples));
}

void AutoLevel::setTargetGain(float newGain)
{
    // 安全装置：極端な爆音や無音によるゲイン暴走を防ぐため、補正範囲を -24dB 〜 +24dB に制限
    float clampedGain = std::clamp(newGain, 0.063f, 15.8f);
    outputGain.setTargetValue(clampedGain);
}

float AutoLevel::processApplication(float processedSignal)
{
    // 出力信号にスタティックなゲインを掛け合わせる
    return processedSignal * outputGain.getNextValue();
}