#pragma once
#include <JuceHeader.h>
#include "../../PluginProcessor.h"

class TransformerSection : public juce::Component
{
public:
    TransformerSection(NeotoPreAudioProcessor& p);
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    NeotoPreAudioProcessor& audioProcessor;

    juce::Slider colorSlider, airSlider, ageSlider;
    juce::Label colorLabel, airLabel, ageLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> colorAttach, airAttach, ageAttach;
    juce::ComboBox outTransCombo;
    juce::Label outTransLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> outTransAttach;

    void setupRotarySlider(juce::Slider& slider, juce::Label& label, const juce::String& name);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransformerSection)
};