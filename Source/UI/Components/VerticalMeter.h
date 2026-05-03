#pragma once
#include <JuceHeader.h>

class VerticalMeter : public juce::Component, public juce::Timer
{
public:
    VerticalMeter();
    ~VerticalMeter() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    // DSPから最新のPeak値(dB)を受け取る関数
    void setLevelDb(float newLevelDb);

private:
    float currentLevelDb = -60.0f;
    float targetLevelDb = -60.0f;
    float peakHoldDb = -60.0f;
    int peakHoldTimer = 0;

    juce::ColourGradient gradient;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VerticalMeter)
};