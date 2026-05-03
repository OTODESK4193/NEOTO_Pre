#include "AutoLevel.h"

void AutoLevel::prepare(double sampleRate)
{
    fs = sampleRate;

    // 192kHzで最大10秒分のバッファを確保
    const int maxSamples = static_cast<int>(std::ceil(192000.0 * 10.0));
    dryHistoryBuffer.assign(maxSamples, 0.0f);
    wetHistoryBuffer.assign(maxSamples, 0.0f);
    dryWriteIndex = 0;
    wetWriteIndex = 0;

    latestDryRms.store(0.0f);
    latestWetRms.store(0.0f);

    // ==============================================================================
    // ITU-R BS.1770 K-Weighting フィルターの初期化
    // ==============================================================================
    juce::dsp::ProcessSpec spec{ sampleRate, 1, 1 };

    // Stage 1: 頭部音響効果シミュレーション (1500Hz, Q=0.7071, Gain=+4.0dB)
    auto preCoefs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, 1500.0f, 0.7071f, juce::Decibels::decibelsToGain(4.0f));

    // Stage 2: 低域ロールオフ (38Hz, Q=0.5)
    auto rlbCoefs = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 38.0f, 0.5f);

    preFilterDry.prepare(spec);
    preFilterDry.coefficients = preCoefs;
    rlbFilterDry.prepare(spec);
    rlbFilterDry.coefficients = rlbCoefs;

    preFilterWet.prepare(spec);
    preFilterWet.coefficients = preCoefs;
    rlbFilterWet.prepare(spec);
    rlbFilterWet.coefficients = rlbCoefs;

    preFilterDry.reset();
    rlbFilterDry.reset();
    preFilterWet.reset();
    rlbFilterWet.reset();
}

void AutoLevel::pushDrySample(float input)
{
    float filtered = preFilterDry.processSample(input);
    filtered = rlbFilterDry.processSample(filtered);

    // Analyze時のCPU負荷をゼロにするため、この段階で二乗（Square）して保存
    dryHistoryBuffer[dryWriteIndex] = filtered * filtered;
    dryWriteIndex = (dryWriteIndex + 1) % dryHistoryBuffer.size();
}

void AutoLevel::pushWetSample(float input)
{
    float filtered = preFilterWet.processSample(input);
    filtered = rlbFilterWet.processSample(filtered);

    wetHistoryBuffer[wetWriteIndex] = filtered * filtered;
    wetWriteIndex = (wetWriteIndex + 1) % wetHistoryBuffer.size();
}

void AutoLevel::analyzeRMS(float seconds)
{
    int numSamples = static_cast<int>(fs * seconds);
    int bufferSize = static_cast<int>(dryHistoryBuffer.size());

    if (numSamples > bufferSize) numSamples = bufferSize;
    if (numSamples <= 0) {
        latestDryRms.store(0.0f);
        latestWetRms.store(0.0f);
        return;
    }

    double sumSquaresDry = 0.0;
    double sumSquaresWet = 0.0;

    int readIdxDry = dryWriteIndex - 1;
    int readIdxWet = wetWriteIndex - 1;

    for (int i = 0; i < numSamples; ++i)
    {
        if (readIdxDry < 0) readIdxDry += bufferSize;
        if (readIdxWet < 0) readIdxWet += bufferSize;

        sumSquaresDry += static_cast<double>(dryHistoryBuffer[readIdxDry]);
        sumSquaresWet += static_cast<double>(wetHistoryBuffer[readIdxWet]);

        readIdxDry--;
        readIdxWet--;
    }

    // ==============================================================================
    // ITU-R BS.1770 エネルギー（Mean Square）としての出力
    // ==============================================================================
    // ※平方根を取らず、純粋なエネルギー値としてProcessorへ渡す
    float msDry = static_cast<float>(sumSquaresDry / numSamples);
    float msWet = static_cast<float>(sumSquaresWet / numSamples);

    latestDryRms.store(msDry);
    latestWetRms.store(msWet);
}