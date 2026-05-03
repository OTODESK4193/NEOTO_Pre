#pragma once
#include <JuceHeader.h>
#include <vector>
#include <atomic>

class AutoLevel
{
public:
    AutoLevel() = default;
    ~AutoLevel() = default;

    void prepare(double sampleRate);

    // ヒストリーバッファへの記録 (内部でK-Weightingと二乗処理を完了させる)
    void pushDrySample(float input);
    void pushWetSample(float input);

    // 指定秒数遡ってK-Weighted RMS(Mean Square)を計算
    void analyzeRMS(float seconds);

    // GUI/Processor表示用: 最新の解析結果を取得 (平方根を取る前の「純粋なエネルギー値」)
    float getLatestDryRMS() const { return latestDryRms.load(); }
    float getLatestWetRMS() const { return latestWetRms.load(); }

private:
    double fs = 44100.0;

    std::vector<float> dryHistoryBuffer;
    std::vector<float> wetHistoryBuffer;
    int dryWriteIndex = 0;
    int wetWriteIndex = 0;

    std::atomic<float> latestDryRms{ 0.0f };
    std::atomic<float> latestWetRms{ 0.0f };

    // ITU-R BS.1770 K-Weighting Filters
    juce::dsp::IIR::Filter<float> preFilterDry;
    juce::dsp::IIR::Filter<float> rlbFilterDry;
    juce::dsp::IIR::Filter<float> preFilterWet;
    juce::dsp::IIR::Filter<float> rlbFilterWet;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutoLevel)
};