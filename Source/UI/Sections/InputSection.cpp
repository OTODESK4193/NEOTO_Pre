#include "InputSection.h"

InputSection::InputSection(NeotoPreAudioProcessor& p) : audioProcessor(p)
{
    setupRotarySlider(inputGainSlider, inputGainLabel, "Input Gain");

    osModeCombo.addItemList({ "Off (1x)", "2x", "4x" }, 1);
    addAndMakeVisible(osModeCombo);

    osModeLabel.setText("Oversampling", juce::dontSendNotification);
    osModeLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(osModeLabel);

    // 追加：入力トランス選択用コンボボックス
    inTransCombo.addItemList({ "None", "Nickel", "Steel", "Iron", "Amorphous" }, 1);
    addAndMakeVisible(inTransCombo);
    inTransLabel.setText("Input Transformer", juce::dontSendNotification);
    inTransLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(inTransLabel);

    inputGainAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "input_gain", inputGainSlider);
    osModeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "os_mode", osModeCombo);
    // APVTSのアタッチメント（後でProcessor側に追加します）
    inTransAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "in_trans_type", inTransCombo);
}

void InputSection::setupRotarySlider(juce::Slider& slider, juce::Label& label, const juce::String& name)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(slider);
    label.setText(name, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(label);
}

void InputSection::paint(juce::Graphics& g)
{
    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 8.0f);
    g.setColour(juce::Colours::grey);
    g.setFont(15.0f);
    g.drawText("INPUT STAGE", getLocalBounds().removeFromTop(30), juce::Justification::centred, false);
}

void InputSection::resized()
{
    auto area = getLocalBounds().reduced(5, 5);
    area.removeFromTop(30);

    // 上段：Input Gain
    auto topRow = area.removeFromTop(120);
    inputGainLabel.setBounds(topRow.removeFromTop(20));
    inputGainSlider.setBounds(topRow.reduced(5));

    // 中段：OS Mode
    auto midRow = area.removeFromTop(60);
    osModeLabel.setBounds(midRow.removeFromTop(20).withSizeKeepingCentre(120, 20));
    osModeCombo.setBounds(midRow.removeFromTop(24).withSizeKeepingCentre(120, 24));

    // 下段：Input Transformer
    auto bottomRow = area;
    inTransLabel.setBounds(bottomRow.removeFromTop(20).withSizeKeepingCentre(160, 20));
    inTransCombo.setBounds(bottomRow.removeFromTop(24).withSizeKeepingCentre(140, 24));
}