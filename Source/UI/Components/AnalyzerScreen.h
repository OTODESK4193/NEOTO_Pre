#pragma once
#include <JuceHeader.h>

class AnalyzerScreen : public juce::Component
{
public:
    AnalyzerScreen();
    ~AnalyzerScreen() override;

    void paint(juce::Graphics& g) override;

private:
    // 対数スケール（Hz）のX座標を計算する関数
    float getPositionForFrequency(float freq, float width);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AnalyzerScreen)
};