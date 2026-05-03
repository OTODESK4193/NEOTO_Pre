#include "InputSection.h"

InputSection::InputSection(NeotoPreAudioProcessor& p) : audioProcessor(p)
{
    setupRotarySlider(inputGainSlider, inputGainLabel, "Input Gain");
    inputGainSlider.setLookAndFeel(&arcLnF);

    osModeCombo.addItemList({ "Off (1x)", "2x", "4x", "8x" }, 1); // ★ "8x" を追加
    addAndMakeVisible(osModeCombo);

    osModeLabel.setText("Oversampling", juce::dontSendNotification);
    osModeLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(osModeLabel);

    inTransCombo.addItemList({ "None", "Nickel", "Steel", "Iron", "Amorphous" }, 1);
    addAndMakeVisible(inTransCombo);
    inTransLabel.setText("Input Transformer", juce::dontSendNotification);
    inTransLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(inTransLabel);

    inputGainAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "input_gain", inputGainSlider);
    osModeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "os_mode", osModeCombo);
    inTransAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "in_trans_type", inTransCombo);

    // ダイナミックカラー（コンボボックス連動）
    auto updateColors = [this] {
        int id = inTransCombo.getSelectedItemIndex();
        juce::Colour c = juce::Colours::cyan;
        if (id == 0) c = juce::Colour(0xff888888); // None: Grey
        else if (id == 1) c = juce::Colour(0xff00d4ff); // Nickel: Cyan
        else if (id == 2) c = juce::Colour(0xffff5555); // Steel: Red
        else if (id == 3) c = juce::Colour(0xffffaa00); // Iron: Orange
        else if (id == 4) c = juce::Colour(0xffcc55ff); // Amorph: Purple
        inputGainSlider.setColour(juce::Slider::rotarySliderFillColourId, c);
        repaint();
        };
    inTransCombo.onChange = updateColors;
    updateColors();
}

InputSection::~InputSection()
{
    inputGainSlider.setLookAndFeel(nullptr); // 安全な破棄
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
    g.setFont(juce::FontOptions(15.0f));
    g.drawText("INPUT STAGE", getLocalBounds().removeFromTop(30), juce::Justification::centred, false);
}

void InputSection::resized()
{
    auto area = getLocalBounds().reduced(5, 5);
    area.removeFromTop(25);

    auto topRow = area.removeFromTop(120);
    inputGainLabel.setBounds(topRow.removeFromTop(20));
    inputGainSlider.setBounds(topRow.withSizeKeepingCentre(85, 85)); // 巨大化

    auto midRow = area.removeFromTop(80);
    osModeLabel.setBounds(midRow.removeFromTop(20).withSizeKeepingCentre(120, 20));
    osModeCombo.setBounds(midRow.removeFromTop(24).withSizeKeepingCentre(120, 24));

    auto bottomRow = area;
    inTransLabel.setBounds(bottomRow.removeFromTop(20).withSizeKeepingCentre(140, 20));
    inTransCombo.setBounds(bottomRow.removeFromTop(24).withSizeKeepingCentre(140, 24));
}