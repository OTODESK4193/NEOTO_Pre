#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class NeotoPreAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    NeotoPreAudioProcessorEditor(NeotoPreAudioProcessor&);
    ~NeotoPreAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    NeotoPreAudioProcessor& audioProcessor;

    // スライダーとUIコンポーネント
    juce::Slider driveSlider, charSlider, asymSlider;
    juce::Slider colorSlider, airSlider, ageSlider;
    juce::Slider targetLufsSlider;

    juce::ComboBox timeCombo;
    juce::TextButton applyButton;

    // ラベル
    juce::Label driveLabel, charLabel, asymLabel;
    juce::Label colorLabel, airLabel, ageLabel;
    juce::Label targetLufsLabel, timeComboLabel;

    // APVTS同期アタッチメント
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<SliderAttachment> driveAttach, charAttach, asymAttach;
    std::unique_ptr<SliderAttachment> colorAttach, airAttach, ageAttach;
    std::unique_ptr<SliderAttachment> targetLufsAttach;
    std::unique_ptr<ComboBoxAttachment> timeAttach;

    void setupRotarySlider(juce::Slider& slider, juce::Label& label, const juce::String& name);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NeotoPreAudioProcessorEditor)
};