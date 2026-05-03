#include "AnalyzeSection.h"

AnalyzeSection::AnalyzeSection(NeotoPreAudioProcessor& p) : audioProcessor(p)
{
    timeCombo.addItemList({ "1 Second", "3 Seconds", "5 Seconds", "10 Seconds" }, 1);
    addAndMakeVisible(timeCombo);

    timeComboLabel.setText("Analyze Time", juce::dontSendNotification);
    timeComboLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(timeComboLabel);

    analyzeButton.setButtonText("Analyze");
    analyzeButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkcyan);
    analyzeButton.onClick = [this] {
        audioProcessor.triggerAnalyze = true;
        dryResultLabel.setText("Analyzing...", juce::dontSendNotification);
        wetResultLabel.setText("", juce::dontSendNotification);
        suggestResultLabel.setText("", juce::dontSendNotification);
        };
    addAndMakeVisible(analyzeButton);

    applyButton.setButtonText("Apply Match");
    applyButton.setColour(juce::TextButton::buttonColourId, juce::Colours::forestgreen);
    applyButton.onClick = [this] {
        float suggestionDb = audioProcessor.latestAnalysisResult.suggestedGainDb;
        float newDb = std::clamp(suggestionDb, -24.0f, 24.0f);
        auto* param = audioProcessor.apvts.getParameter("output_gain");
        param->setValueNotifyingHost(param->convertTo0to1(newDb));
        suggestResultLabel.setText("Applied!", juce::dontSendNotification);
        };
    addAndMakeVisible(applyButton);

    auto setupResultLabel = [this](juce::Label& lbl) {
        lbl.setFont(14.0f);
        lbl.setJustificationType(juce::Justification::centred);
        lbl.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
        addAndMakeVisible(lbl);
        };

    setupResultLabel(dryResultLabel);
    setupResultLabel(wetResultLabel);
    setupResultLabel(suggestResultLabel);

    dryResultLabel.setText("Dry: -- LUFS", juce::dontSendNotification);
    wetResultLabel.setText("Wet: -- LUFS", juce::dontSendNotification);
    suggestResultLabel.setText("Suggest: -- dB", juce::dontSendNotification);

    timeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "analysis_time", timeCombo);

    startTimerHz(10);
}

AnalyzeSection::~AnalyzeSection()
{
    stopTimer();
}

void AnalyzeSection::timerCallback()
{
    if (audioProcessor.hasNewAnalysisResult.exchange(false))
    {
        auto& res = audioProcessor.latestAnalysisResult;

        // Processor側ですでに Lufs(dB) に計算されている値をそのまま受け取る (修正の要)
        float dryLufs = res.dryRmsL;
        float wetLufs = res.wetRmsL;

        dryResultLabel.setText(juce::String::formatted("Dry: %.1f LUFS", dryLufs), juce::dontSendNotification);
        wetResultLabel.setText(juce::String::formatted("Wet: %.1f LUFS", wetLufs), juce::dontSendNotification);

        juce::String sign = res.suggestedGainDb > 0.0f ? "+" : "";
        juce::String suggestText = "Suggest: " + sign + juce::String(res.suggestedGainDb, 1) + " dB";

        suggestResultLabel.setText(suggestText, juce::dontSendNotification);
        suggestResultLabel.setColour(juce::Label::textColourId, juce::Colours::lightgreen);
    }
}

void AnalyzeSection::paint(juce::Graphics& g)
{
    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 8.0f);
    g.setColour(juce::Colours::grey);
    g.setFont(15.0f);
    g.drawText("GAIN MATCHING", getLocalBounds().removeFromTop(30), juce::Justification::centred, false);
}

void AnalyzeSection::resized()
{
    auto area = getLocalBounds().reduced(5, 5);
    area.removeFromTop(30);

    auto topLevel = area.removeFromTop(60);
    timeComboLabel.setBounds(topLevel.removeFromTop(15).withSizeKeepingCentre(100, 15));
    timeCombo.setBounds(topLevel.removeFromTop(24).withSizeKeepingCentre(100, 24));

    auto midLevel = area.removeFromTop(40);
    analyzeButton.setBounds(midLevel.withSizeKeepingCentre(120, 30));

    auto bottomLevel = area;
    dryResultLabel.setBounds(bottomLevel.removeFromTop(20));
    wetResultLabel.setBounds(bottomLevel.removeFromTop(20));
    suggestResultLabel.setBounds(bottomLevel.removeFromTop(25));
    applyButton.setBounds(bottomLevel.removeFromTop(35).withSizeKeepingCentre(140, 30));
}