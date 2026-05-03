#pragma once
#include <JuceHeader.h>
#include "../../PluginProcessor.h"

class OutputSection : public juce::Component, public juce::Timer
{
public:
    OutputSection(NeotoPreAudioProcessor& p);
    ~OutputSection() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    NeotoPreAudioProcessor& audioProcessor;

    void setupRotarySlider(juce::Slider& slider, juce::Label& label, const juce::String& name);

    juce::Slider outGainSlider;
    juce::Slider mixSlider;
    juce::Label outGainLabel;
    juce::Label mixLabel;
    juce::ToggleButton listenButton;

    // Gain Matching 統合パーツ
    juce::ComboBox timeCombo;
    juce::Label timeComboLabel;
    juce::TextButton analyzeButton;
    juce::TextButton applyButton;
    juce::Label dryResultLabel;
    juce::Label wetResultLabel;
    juce::Label suggestResultLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outGainAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> listenAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> timeAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutputSection)
};