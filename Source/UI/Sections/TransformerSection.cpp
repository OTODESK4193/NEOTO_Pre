#include "TransformerSection.h"

TransformerSection::TransformerSection(NeotoPreAudioProcessor& p) : audioProcessor(p)
{
    setupRotarySlider(colorSlider, colorLabel, "Color");
    setupRotarySlider(airSlider, airLabel, "Air");
    setupRotarySlider(ageSlider, ageLabel, "Age");

    colorSlider.setLookAndFeel(&arcLnF);
    airSlider.setLookAndFeel(&arcLnF);
    ageSlider.setLookAndFeel(&arcLnF);

    // ★ Carnhill, Cinemag を追加 (合計7項目)
    outTransCombo.addItemList({ "None", "Nickel", "Steel", "Iron", "Amorphous", "Carnhill", "Cinemag" }, 1);
    addAndMakeVisible(outTransCombo);
    outTransLabel.setText("Output Transformer", juce::dontSendNotification);
    outTransLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(outTransLabel);

    colorAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "color", colorSlider);
    airAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "air", airSlider);
    ageAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "age", ageSlider);
    outTransAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "out_trans_type", outTransCombo);

    // ダイナミックカラー
    auto updateColors = [this] {
        int id = outTransCombo.getSelectedItemIndex();
        juce::Colour c = juce::Colours::cyan;
        if (id == 0)      c = juce::Colour(0xff444444);      // None → チャコール
        else if (id == 1) c = juce::Colour(0xff0088aa);      // Nickel → ティール
        else if (id == 2) c = juce::Colour(0xffaa2222);      // Steel → 濃赤
        else if (id == 3) c = juce::Colour(0xffcc5500);      // Iron → 濃オレンジ
        else if (id == 4) c = juce::Colour(0xff7722cc);      // Amorphous → 深紫
        else if (id == 5) c = juce::Colour(0xffbbbbbb);      // Carnhill → シルバー
        else if (id == 6) c = juce::Colour(0xff0055dd);      // Cinemag → 濃青

        colorSlider.setColour(juce::Slider::rotarySliderFillColourId, c);
        airSlider.setColour(juce::Slider::rotarySliderFillColourId, c);
        ageSlider.setColour(juce::Slider::rotarySliderFillColourId, c);
        repaint();
        };
    outTransCombo.onChange = updateColors;
    updateColors();
}

TransformerSection::~TransformerSection()
{
    colorSlider.setLookAndFeel(nullptr);
    airSlider.setLookAndFeel(nullptr);
    ageSlider.setLookAndFeel(nullptr);
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
    g.setFont(juce::FontOptions(15.0f));
    g.drawText("OUTPUT TRANSFORMER", getLocalBounds().removeFromTop(30), juce::Justification::centred, false);
}

void TransformerSection::resized()
{
    auto area = getLocalBounds().reduced(5, 5);
    area.removeFromTop(25);

    auto topRow = area.removeFromTop(120);
    auto slot1 = topRow.removeFromLeft(topRow.getWidth() / 2);
    colorLabel.setBounds(slot1.removeFromTop(20));
    colorSlider.setBounds(slot1.withSizeKeepingCentre(85, 85)); // 巨大化

    auto slot2 = topRow;
    airLabel.setBounds(slot2.removeFromTop(20));
    airSlider.setBounds(slot2.withSizeKeepingCentre(85, 85)); // 巨大化

    auto midRow = area.removeFromTop(100);
    ageLabel.setBounds(midRow.removeFromTop(20));
    ageSlider.setBounds(midRow.withSizeKeepingCentre(85, 85)); // 巨大化

    auto bottomRow = area;
    outTransLabel.setBounds(bottomRow.removeFromTop(20).withSizeKeepingCentre(140, 20));
    outTransCombo.setBounds(bottomRow.removeFromTop(24).withSizeKeepingCentre(140, 24));
}