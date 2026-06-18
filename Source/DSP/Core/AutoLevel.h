#pragma once
#include <JuceHeader.h>
#include <atomic>

class AutoLevel
{
public:
    AutoLevel() = default;
    ~AutoLevel() = default;

    void prepare(double sampleRate);

    // ==============================================================================
    // 録音制御（Message Thread から呼ばれる）
    // ==============================================================================
    void startRecording(int totalSamples);

    // 録音中かどうか（Audio Thread / Message Thread 両方から読み取り可能）
    bool isRecording() const { return recording.load(std::memory_order_relaxed); }

    // 録音完了フラグ（Message Thread が読み取り → exchange で消費）
    bool isComplete() const { return recordingComplete.load(std::memory_order_relaxed); }
    void clearComplete() { recordingComplete.store(false); }

    // ==============================================================================
    // サンプル記録（Audio Thread から毎サンプル呼ばれる）
    // ==============================================================================
    void pushDrySample(float input);
    void pushWetSample(float input);

    // ==============================================================================
    // 解析結果取得（録音完了後、Message Thread から読み取り）
    // ==============================================================================
    float getDryMeanSquare() const { return dryMeanSquare.load(); }
    float getWetMeanSquare() const { return wetMeanSquare.load(); }

private:
    double fs = 44100.0;

    // 録音状態管理
    std::atomic<bool> recording{ false };
    std::atomic<bool> recordingComplete{ false };
    std::atomic<int> samplesToRecord{ 0 };
    int samplesRecorded = 0;

    // リアルタイム累積バッファ（Audio Thread のみ書き込み）
    double drySquareSum = 0.0;
    double wetSquareSum = 0.0;
    int wetSamplesRecorded = 0;

    // 解析結果（Audio Thread が書き込み、Message Thread が読み取り）
    std::atomic<float> dryMeanSquare{ 0.0f };
    std::atomic<float> wetMeanSquare{ 0.0f };

    // ITU-R BS.1770 K-Weighting Filters
    juce::dsp::IIR::Filter<float> preFilterDry;
    juce::dsp::IIR::Filter<float> rlbFilterDry;
    juce::dsp::IIR::Filter<float> preFilterWet;
    juce::dsp::IIR::Filter<float> rlbFilterWet;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutoLevel)
};