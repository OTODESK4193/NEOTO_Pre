#pragma once
#include <JuceHeader.h>
#include "../../PluginProcessor.h"
#include "../../GUI/ArcDial.h" 

class TransformerSection : public juce::Component
{
public:
    TransformerSection(NeotoPreAudioProcessor& p);
    ~TransformerSection() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    NeotoPreAudioProcessor& audioProcessor;
    ArcDialLookAndFeel arcLnF;

    void setupRotarySlider(juce::Slider& slider, juce::Label& label, const juce::String& name);

    juce::Slider colorSlider;
    juce::Label colorLabel;
    juce::Slider airSlider;
    juce::Label airLabel;
    juce::Slider ageSlider;
    juce::Label ageLabel;

    juce::ComboBox outTransCombo;
    juce::Label outTransLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> colorAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> airAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> ageAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> outTransAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransformerSection)
};