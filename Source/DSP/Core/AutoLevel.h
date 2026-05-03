#pragma once
#include <JuceHeader.h>
#include <vector>

class AutoLevel
{
public:
    AutoLevel() = default;
    ~AutoLevel() = default;

    void prepare(double sampleRate);

    // フェーズ1: 入力信号をヒストリーバッファに記録する（信号自体は変更・遅延させない）
    void pushSample(float input);

    // Applyボタンが押された時、指定された秒数遡ってRMSを計算する
    float calculateRMS(float seconds) const;

    // 計算された目標ゲインをセットする（適用時は滑らかにランプさせる）
    void setTargetGain(float newGain);

    // フェーズ5: 最終的なゲインを適用する
    float processApplication(float processedSignal);

private:
    double fs = 44100.0;

    // 過去の波形を保持するリングバッファ
    std::vector<float> historyBuffer;
    int writeIndex = 0;

    // 滑らかなゲイン変更のためのスムーザー
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> outputGain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutoLevel)
};