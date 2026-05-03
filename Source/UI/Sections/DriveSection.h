#pragma once
#include <JuceHeader.h>
#include "../../PluginProcessor.h"
#include "../../GUI/ArcDial.h" // 追加

class DriveSection : public juce::Component
{
public:
    DriveSection(NeotoPreAudioProcessor& p);
    ~DriveSection() override; // デストラクタの宣言を追加！

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    NeotoPreAudioProcessor& audioProcessor;
    ArcDialLookAndFeel arcLnF; // スキンを追加

    void setupRotarySlider(juce::Slider& slider, juce::Label& label, const juce::String& name);

    juce::Slider driveSlider;
    juce::Label driveLabel;
    juce::Slider charSlider;
    juce::Label charLabel;
    juce::Slider asymSlider;
    juce::Label asymLabel;

    juce::ComboBox preampModelCombo;
    juce::Label preampModelLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> driveAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> charAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> asymAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> preampModelAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DriveSection)
};