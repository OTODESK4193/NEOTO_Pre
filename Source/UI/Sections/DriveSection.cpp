#include "DriveSection.h"

DriveSection::DriveSection(NeotoPreAudioProcessor& p) : audioProcessor(p)
{
    setupRotarySlider(driveSlider, driveLabel, "Drive");
    setupRotarySlider(charSlider, charLabel, "Character");
    setupRotarySlider(asymSlider, asymLabel, "Asymmetry");

    // 追加：プリアンプモデル選択用コンボボックス
    preampModelCombo.addItemList({ "API Style", "Neve Style", "Vintage Tube", "SSL Modern", "Modern 1", "Modern 2" }, 1);
    addAndMakeVisible(preampModelCombo);
    preampModelLabel.setText("Preamp Model", juce::dontSendNotification);
    preampModelLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(preampModelLabel);

    driveAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "drive", driveSlider);
    charAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "character", charSlider);
    asymAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "asymmetry", asymSlider);

    // APVTSのアタッチメント
    preampModelAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "preamp_model", preampModelCombo);
}

void DriveSection::setupRotarySlider(juce::Slider& slider, juce::Label& label, const juce::String& name)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(slider);
    label.setText(name, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(label);
}

void DriveSection::paint(juce::Graphics& g)
{
    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 8.0f);
    g.setColour(juce::Colours::grey);
    g.setFont(15.0f);
    g.drawText("DRIVE STAGE", getLocalBounds().removeFromTop(30), juce::Justification::centred, false);
}

void DriveSection::resized()
{
    auto area = getLocalBounds().reduced(5, 5);
    area.removeFromTop(30);

    // 上段：Drive & Character
    auto topRow = area.removeFromTop(120);
    auto slot1 = topRow.removeFromLeft(topRow.getWidth() / 2);
    driveLabel.setBounds(slot1.removeFromTop(20));
    driveSlider.setBounds(slot1.reduced(5));

    auto slot2 = topRow;
    charLabel.setBounds(slot2.removeFromTop(20));
    charSlider.setBounds(slot2.reduced(5));

    // 中段：Asymmetry
    auto midRow = area.removeFromTop(100);
    auto slot3 = midRow.withSizeKeepingCentre(100, midRow.getHeight());
    asymLabel.setBounds(slot3.removeFromTop(20));
    asymSlider.setBounds(slot3.reduced(5));

    // 下段：Preamp Model
    auto bottomRow = area;
    preampModelLabel.setBounds(bottomRow.removeFromTop(20).withSizeKeepingCentre(160, 20));
    preampModelCombo.setBounds(bottomRow.removeFromTop(24).withSizeKeepingCentre(140, 24));
}