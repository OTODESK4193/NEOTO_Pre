#include "DriveSection.h"

DriveSection::DriveSection(NeotoPreAudioProcessor& p) : audioProcessor(p)
{
    setupRotarySlider(driveSlider, driveLabel, "Drive");
    setupRotarySlider(charSlider, charLabel, "Character");
    setupRotarySlider(asymSlider, asymLabel, "Asymmetry");

    driveSlider.setLookAndFeel(&arcLnF);
    charSlider.setLookAndFeel(&arcLnF);
    asymSlider.setLookAndFeel(&arcLnF);

    preampModelCombo.addItemList({ "API Style", "Neve Style", "Vintage Tube", "SSL Modern", "Modern 1", "Modern 2" }, 1);
    addAndMakeVisible(preampModelCombo);
    preampModelLabel.setText("Preamp Model", juce::dontSendNotification);
    preampModelLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(preampModelLabel);

    driveAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "drive", driveSlider);
    charAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "character", charSlider);
    asymAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "asymmetry", asymSlider);
    preampModelAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "preamp_model", preampModelCombo);

    // ダイナミックカラー
    auto updateColors = [this] {
        int id = preampModelCombo.getSelectedItemIndex();
        juce::Colour c = juce::Colours::cyan;
        if (id == 0) c = juce::Colour(0xff00d4ff); // API: Blue
        else if (id == 1) c = juce::Colour(0xffff5555); // Neve: Red
        else if (id == 2) c = juce::Colour(0xffffaa00); // Tube: Orange
        else if (id == 3) c = juce::Colour(0xff00ff88); // SSL: Green
        else if (id == 4) c = juce::Colours::white;     // Modern1
        else if (id == 5) c = juce::Colour(0xffcc55ff); // Modern2

        driveSlider.setColour(juce::Slider::rotarySliderFillColourId, c);
        charSlider.setColour(juce::Slider::rotarySliderFillColourId, c);
        asymSlider.setColour(juce::Slider::rotarySliderFillColourId, c);
        repaint();
        };
    preampModelCombo.onChange = updateColors;
    updateColors();
}

DriveSection::~DriveSection()
{
    driveSlider.setLookAndFeel(nullptr);
    charSlider.setLookAndFeel(nullptr);
    asymSlider.setLookAndFeel(nullptr);
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
    g.setFont(juce::FontOptions(15.0f));
    g.drawText("DRIVE STAGE", getLocalBounds().removeFromTop(30), juce::Justification::centred, false);
}

void DriveSection::resized()
{
    auto area = getLocalBounds().reduced(5, 5);
    area.removeFromTop(25);

    auto topRow = area.removeFromTop(120);
    auto slot1 = topRow.removeFromLeft(topRow.getWidth() / 2);
    driveLabel.setBounds(slot1.removeFromTop(20));
    driveSlider.setBounds(slot1.withSizeKeepingCentre(85, 85)); // 巨大化

    auto slot2 = topRow;
    charLabel.setBounds(slot2.removeFromTop(20));
    charSlider.setBounds(slot2.withSizeKeepingCentre(85, 85)); // 巨大化

    auto midRow = area.removeFromTop(100);
    asymLabel.setBounds(midRow.removeFromTop(20));
    asymSlider.setBounds(midRow.withSizeKeepingCentre(85, 85)); // 巨大化

    auto bottomRow = area;
    preampModelLabel.setBounds(bottomRow.removeFromTop(20).withSizeKeepingCentre(140, 20));
    preampModelCombo.setBounds(bottomRow.removeFromTop(24).withSizeKeepingCentre(140, 24));
}