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

    // ★ 変更: ダイナミックカラーの更新と、スマート・デフォルト機能（自動トランス切り替え）の統合
    auto updateColorsAndRouting = [this] {
        int id = preampModelCombo.getSelectedItemIndex();
        juce::Colour c = juce::Colours::cyan;

        // 1. カラーの更新
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

        // 2. スマート・デフォルト (Smart Defaulting)
        auto* inTransParam = audioProcessor.apvts.getParameter("in_trans_type");
        auto* outTransParam = audioProcessor.apvts.getParameter("out_trans_type");

        if (inTransParam && outTransParam) {
            // (0:None, 1:Nickel, 2:Steel, 3:Iron, 4:Amorphous)
            if (id == 0) { // API Style
                inTransParam->setValueNotifyingHost(inTransParam->convertTo0to1(3)); // Iron
                outTransParam->setValueNotifyingHost(outTransParam->convertTo0to1(2)); // Steel
            }
            else if (id == 1) { // Neve Style
                inTransParam->setValueNotifyingHost(inTransParam->convertTo0to1(1)); // Nickel
                outTransParam->setValueNotifyingHost(outTransParam->convertTo0to1(2)); // Steel
            }
            else if (id == 2) { // Vintage Tube
                inTransParam->setValueNotifyingHost(inTransParam->convertTo0to1(1)); // Nickel
                outTransParam->setValueNotifyingHost(outTransParam->convertTo0to1(1)); // Nickel
            }
            else { // SSL / Modern 1 / Modern 2
                inTransParam->setValueNotifyingHost(inTransParam->convertTo0to1(0)); // None
                outTransParam->setValueNotifyingHost(outTransParam->convertTo0to1(0)); // None
            }
        }
        };

    preampModelCombo.onChange = updateColorsAndRouting;

    // UI初期化時のカラー設定のみ実行 (パラメータの書き換えはトリガーしない)
    int initialId = preampModelCombo.getSelectedItemIndex();
    juce::Colour initialColor = (initialId == 0) ? juce::Colour(0xff00d4ff) :
        (initialId == 1) ? juce::Colour(0xffff5555) : juce::Colours::cyan;
    driveSlider.setColour(juce::Slider::rotarySliderFillColourId, initialColor);
    charSlider.setColour(juce::Slider::rotarySliderFillColourId, initialColor);
    asymSlider.setColour(juce::Slider::rotarySliderFillColourId, initialColor);
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