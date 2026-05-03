#pragma once
#include <JuceHeader.h>
#include "../../PluginProcessor.h"

// タイマー処理はこのクラスだけが担当します
class AnalyzeSection : public juce::Component, public juce::Timer
{
public:
    AnalyzeSection(NeotoPreAudioProcessor& p);
    ~AnalyzeSection() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    NeotoPreAudioProcessor& audioProcessor;

    juce::ComboBox timeCombo;
    juce::TextButton analyzeButton;
    juce::TextButton applyButton;
    juce::Label timeComboLabel;

    juce::Label dryResultLabel;
    juce::Label wetResultLabel;
    juce::Label suggestResultLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> timeAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AnalyzeSection)
};