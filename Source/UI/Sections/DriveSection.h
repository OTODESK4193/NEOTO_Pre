#pragma once
#include <JuceHeader.h>
#include "../../PluginProcessor.h"

class DriveSection : public juce::Component
{
public:
    DriveSection(NeotoPreAudioProcessor& p);
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    NeotoPreAudioProcessor& audioProcessor;

    juce::Slider driveSlider, charSlider, asymSlider;
    juce::Label driveLabel, charLabel, asymLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> driveAttach, charAttach, asymAttach;
    juce::ComboBox preampModelCombo;
    juce::Label preampModelLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> preampModelAttach;

    void setupRotarySlider(juce::Slider& slider, juce::Label& label, const juce::String& name);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DriveSection)
};