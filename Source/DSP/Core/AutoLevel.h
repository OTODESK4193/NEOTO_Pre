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

    // ヒストリーバッファへの記録
    void pushDrySample(float input);
    void pushWetSample(float input);

    // 指定秒数遡ってRMSを計算
    void analyzeRMS(float seconds);

    // GUI表示用: 最新の解析結果を取得
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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutoLevel)
};