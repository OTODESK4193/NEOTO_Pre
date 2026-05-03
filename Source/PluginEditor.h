#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class NeotoPreAudioProcessorEditor : public juce::AudioProcessorEditor,
    public juce::Timer
{
public:
    NeotoPreAudioProcessorEditor(NeotoPreAudioProcessor&);
    ~NeotoPreAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    NeotoPreAudioProcessor& audioProcessor;

    juce::Slider inputGainSlider, outputGainSlider, mixSlider;
    juce::Slider driveSlider, charSlider, asymSlider;
    juce::Slider colorSlider, airSlider, ageSlider;

    juce::ComboBox osModeCombo;
    juce::ComboBox timeCombo;
    juce::TextButton analyzeButton;
    juce::TextButton applyButton;
    juce::TextButton listenButton;

    juce::Label inputGainLabel, outputGainLabel, mixLabel, osModeLabel;
    juce::Label driveLabel, charLabel, asymLabel;
    juce::Label colorLabel, airLabel, ageLabel;
    juce::Label timeComboLabel;

    juce::Label dryResultLabel;
    juce::Label wetResultLabel;
    juce::Label suggestResultLabel;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> inputGainAttach, outputGainAttach, mixAttach;
    std::unique_ptr<SliderAttachment> driveAttach, charAttach, asymAttach;
    std::unique_ptr<SliderAttachment> colorAttach, airAttach, ageAttach;
    std::unique_ptr<ComboBoxAttachment> osModeAttach, timeAttach;
    std::unique_ptr<ButtonAttachment> listenAttach;

    void setupRotarySlider(juce::Slider& slider, juce::Label& label, const juce::String& name);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NeotoPreAudioProcessorEditor)
};