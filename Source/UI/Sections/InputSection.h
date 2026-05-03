#pragma once
#include <JuceHeader.h>
#include "../../PluginProcessor.h"

class InputSection : public juce::Component
{
public:
    InputSection(NeotoPreAudioProcessor& p);
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    NeotoPreAudioProcessor& audioProcessor;

    juce::Slider inputGainSlider;
    juce::ComboBox osModeCombo;
    juce::Label inputGainLabel, osModeLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inputGainAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> osModeAttach;
    juce::ComboBox inTransCombo;
    juce::Label inTransLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> inTransAttach;

    void setupRotarySlider(juce::Slider& slider, juce::Label& label, const juce::String& name);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InputSection)
};