#pragma once
#include <JuceHeader.h>
#include "../../PluginProcessor.h"
#include "../../GUI/ArcDial.h" // 追加

class InputSection : public juce::Component
{
public:
    InputSection(NeotoPreAudioProcessor& p);
    ~InputSection() override; // デストラクタの宣言を追加！

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    NeotoPreAudioProcessor& audioProcessor;
    ArcDialLookAndFeel arcLnF; // スキンを追加

    void setupRotarySlider(juce::Slider& slider, juce::Label& label, const juce::String& name);

    juce::Slider inputGainSlider;
    juce::Label inputGainLabel;
    juce::ComboBox osModeCombo;
    juce::Label osModeLabel;
    juce::ComboBox inTransCombo;
    juce::Label inTransLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inputGainAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> osModeAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> inTransAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InputSection)
};