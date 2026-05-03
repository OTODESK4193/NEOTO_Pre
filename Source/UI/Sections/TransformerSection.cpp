#include "TransformerSection.h"

TransformerSection::TransformerSection(NeotoPreAudioProcessor& p) : audioProcessor(p)
{
    setupRotarySlider(colorSlider, colorLabel, "Color");
    setupRotarySlider(airSlider, airLabel, "Air");
    setupRotarySlider(ageSlider, ageLabel, "Age");

    // 追加：出力トランス選択用コンボボックス
    outTransCombo.addItemList({ "None", "Nickel", "Steel", "Iron", "Amorphous" }, 1);
    addAndMakeVisible(outTransCombo);
    outTransLabel.setText("Output Transformer", juce::dontSendNotification);
    outTransLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(outTransLabel);

    colorAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "color", colorSlider);
    airAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "air", airSlider);
    ageAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "age", ageSlider);

    // APVTSのアタッチメント
    outTransAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "out_trans_type", outTransCombo);
}

void TransformerSection::setupRotarySlider(juce::Slider& slider, juce::Label& label, const juce::String& name)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(slider);
    label.setText(name, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(label);
}

void TransformerSection::paint(juce::Graphics& g)
{
    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 8.0f);
    g.setColour(juce::Colours::grey);
    g.setFont(15.0f);
    g.drawText("OUTPUT TRANSFORMER", getLocalBounds().removeFromTop(30), juce::Justification::centred, false); // タイトル変更
}

void TransformerSection::resized()
{
    auto area = getLocalBounds().reduced(5, 5);
    area.removeFromTop(30);
    auto topRow = area.removeFromTop(90);
    auto slot1 = topRow.removeFromLeft(topRow.getWidth() / 2);
    colorLabel.setBounds(slot1.removeFromTop(20));
    colorSlider.setBounds(slot1.withSizeKeepingCentre(65, 65));

    auto slot2 = topRow;
    airLabel.setBounds(slot2.removeFromTop(20));
    airSlider.setBounds(slot2.withSizeKeepingCentre(65, 65));

    auto midRow = area.removeFromTop(90);
    ageLabel.setBounds(midRow.removeFromTop(20));
    ageSlider.setBounds(midRow.withSizeKeepingCentre(65, 65));

    auto bottomRow = area;
    outTransLabel.setBounds(bottomRow.removeFromTop(20).withSizeKeepingCentre(140, 20));
    outTransCombo.setBounds(bottomRow.removeFromTop(24).withSizeKeepingCentre(140, 24));
}