#include "OutputSection.h"

OutputSection::OutputSection(NeotoPreAudioProcessor& p) : audioProcessor(p)
{
    setupRotarySlider(outputGainSlider, outputGainLabel, "Output Gain");
    setupRotarySlider(mixSlider, mixLabel, "Mix (Dry/Wet)");

    listenButton.setButtonText("Listen: DRY (Bypass)");
    listenButton.setClickingTogglesState(true);
    listenButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::darkorange);
    addAndMakeVisible(listenButton);

    outputGainAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "output_gain", outputGainSlider);
    mixAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "mix", mixSlider);
    listenAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, "listen_mode", listenButton);
}

void OutputSection::setupRotarySlider(juce::Slider& slider, juce::Label& label, const juce::String& name)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(slider);
    label.setText(name, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(label);
}

void OutputSection::paint(juce::Graphics& g)
{
    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 8.0f);
    g.setColour(juce::Colours::grey);
    g.setFont(15.0f);
    g.drawText("OUTPUT STAGE", getLocalBounds().removeFromTop(30), juce::Justification::centred, false);
}

void OutputSection::resized()
{
    auto area = getLocalBounds().reduced(5, 5);
    area.removeFromTop(30);

    auto topRow = area.removeFromTop(area.getHeight() / 2);
    auto bottomRow = area;

    auto slot1 = topRow.removeFromLeft(topRow.getWidth() / 2);
    outputGainLabel.setBounds(slot1.removeFromTop(20));
    outputGainSlider.setBounds(slot1.reduced(5));

    auto slot2 = topRow;
    mixLabel.setBounds(slot2.removeFromTop(20));
    mixSlider.setBounds(slot2.reduced(5));

    listenButton.setBounds(bottomRow.withSizeKeepingCentre(160, 30));
}