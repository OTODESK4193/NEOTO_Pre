#include "AutoLevel.h"

void AutoLevel::prepare(double sampleRate)
{
    fs = sampleRate;

    // 状態リセット
    recording.store(false);
    recordingComplete.store(false);
    samplesToRecord.store(0);
    samplesRecorded = 0;
    drySquareSum = 0.0;
    wetSquareSum = 0.0;
    wetSamplesRecorded = 0;
    dryMeanSquare.store(0.0f);
    wetMeanSquare.store(0.0f);

    // ==============================================================================
    // ITU-R BS.1770 K-Weighting フィルターの初期化
    // ==============================================================================
    juce::dsp::ProcessSpec spec{ sampleRate, 1, 1 };

    // Stage 1: 頭部音響効果シミュレーション (1500Hz, Q=0.7071, Gain=+4.0dB)
    auto preCoefs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(
        sampleRate, 1500.0f, 0.7071f, juce::Decibels::decibelsToGain(4.0f));

    // Stage 2: 低域ロールオフ (38Hz, Q=0.5)
    auto rlbCoefs = juce::dsp::IIR::Coefficients<float>::makeHighPass(
        sampleRate, 38.0f, 0.5f);

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

void AutoLevel::startRecording(int totalSamples)
{
    // 一旦停止（AudioThreadが recording=false を見てアクセスを止める）
    recording.store(false);

    // 状態リセット
    drySquareSum = 0.0;
    wetSquareSum = 0.0;
    samplesRecorded = 0;
    wetSamplesRecorded = 0;
    dryMeanSquare.store(0.0f);
    wetMeanSquare.store(0.0f);
    recordingComplete.store(false);
    samplesToRecord.store(totalSamples);

    // K-Weightingフィルターをリセット（新しい測定のため）
    preFilterDry.reset();
    rlbFilterDry.reset();
    preFilterWet.reset();
    rlbFilterWet.reset();

    // 録音開始（最後に設定 → AudioThreadがこのフラグを見て記録を開始する）
    recording.store(true);
}

void AutoLevel::pushDrySample(float input)
{
    if (!recording.load(std::memory_order_relaxed)) return;

    // K-Weighting フィルタリング
    float filtered = preFilterDry.processSample(input);
    filtered = rlbFilterDry.processSample(filtered);

    // 二乗累積
    drySquareSum += static_cast<double>(filtered * filtered);
    samplesRecorded++;

    // 録音完了判定
    if (samplesRecorded >= samplesToRecord.load(std::memory_order_relaxed))
    {
        // 結果を確定
        dryMeanSquare.store(static_cast<float>(drySquareSum / samplesRecorded));
        wetMeanSquare.store(wetSamplesRecorded > 0
            ? static_cast<float>(wetSquareSum / wetSamplesRecorded)
            : 0.0f);

        recording.store(false);
        recordingComplete.store(true);
    }
}

void AutoLevel::pushWetSample(float input)
{
    if (!recording.load(std::memory_order_relaxed)) return;

    // K-Weighting フィルタリング
    float filtered = preFilterWet.processSample(input);
    filtered = rlbFilterWet.processSample(filtered);

    // 二乗累積
    wetSquareSum += static_cast<double>(filtered * filtered);
    wetSamplesRecorded++;
}