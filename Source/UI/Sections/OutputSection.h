#pragma once
#include <JuceHeader.h>
#include "../../PluginProcessor.h"

class OutputSection : public juce::Component
{
public:
    OutputSection(NeotoPreAudioProcessor& p);
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    NeotoPreAudioProcessor& audioProcessor;

    juce::Slider outputGainSlider, mixSlider;
    juce::Label outputGainLabel, mixLabel;
    juce::TextButton listenButton;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputGainAttach, mixAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> listenAttach;

    void setupRotarySlider(juce::Slider& slider, juce::Label& label, const juce::String& name);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutputSection)
};