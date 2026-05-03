#include "OutputSection.h"

OutputSection::OutputSection(NeotoPreAudioProcessor& p) : audioProcessor(p)
{
    setupRotarySlider(outGainSlider, outGainLabel, "Output Gain");
    setupRotarySlider(mixSlider, mixLabel, "Mix (Dry/Wet)");

    listenButton.setButtonText("Listen: DRY (Bypass)");
    addAndMakeVisible(listenButton);

    // Gain Matching UI
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
        lbl.setFont(juce::FontOptions(13.0f)); // ★ここを修正
        lbl.setJustificationType(juce::Justification::centred);
        lbl.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
        addAndMakeVisible(lbl);
        };
    setupResultLabel(dryResultLabel); setupResultLabel(wetResultLabel); setupResultLabel(suggestResultLabel);
    dryResultLabel.setText("Dry: -- LUFS", juce::dontSendNotification);
    wetResultLabel.setText("Wet: -- LUFS", juce::dontSendNotification);
    suggestResultLabel.setText("Suggest: -- dB", juce::dontSendNotification);

    outGainAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "output_gain", outGainSlider);
    mixAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "mix", mixSlider);
    listenAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, "listen_mode", listenButton);
    timeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "analysis_time", timeCombo);

    startTimerHz(10); // LUFS監視用タイマー
}

OutputSection::~OutputSection() { stopTimer(); }

void OutputSection::setupRotarySlider(juce::Slider& slider, juce::Label& label, const juce::String& name) {
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
    addAndMakeVisible(slider);
    label.setText(name, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(label);
}

void OutputSection::timerCallback() {
    if (audioProcessor.hasNewAnalysisResult.exchange(false)) {
        auto& res = audioProcessor.latestAnalysisResult;
        dryResultLabel.setText(juce::String::formatted("Dry: %.1f LUFS", res.dryRmsL), juce::dontSendNotification);
        wetResultLabel.setText(juce::String::formatted("Wet: %.1f LUFS", res.wetRmsL), juce::dontSendNotification);
        juce::String sign = res.suggestedGainDb > 0.0f ? "+" : "";
        suggestResultLabel.setText("Suggest: " + sign + juce::String(res.suggestedGainDb, 1) + " dB", juce::dontSendNotification);
        suggestResultLabel.setColour(juce::Label::textColourId, juce::Colours::lightgreen);
    }
}

void OutputSection::paint(juce::Graphics& g) {
    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 8.0f);
    g.setColour(juce::Colours::grey);
    g.setFont(juce::FontOptions(15.0f)); // ★ここを修正
    g.drawText("OUTPUT & MATCHING", getLocalBounds().removeFromTop(30), juce::Justification::centred, false);
}

void OutputSection::resized() {
    auto area = getLocalBounds().reduced(10);
    area.removeFromTop(20); // タイトル余白

    // 上段: ノブ
    auto knobRow = area.removeFromTop(90);
    auto slot1 = knobRow.removeFromLeft(knobRow.getWidth() / 2);
    outGainLabel.setBounds(slot1.removeFromTop(20));
    outGainSlider.setBounds(slot1.withSizeKeepingCentre(65, 65));

    auto slot2 = knobRow;
    mixLabel.setBounds(slot2.removeFromTop(20));
    mixSlider.setBounds(slot2.withSizeKeepingCentre(65, 65));

    area.removeFromTop(10); // 余白

    // 中段: Analyze設定
    auto timeRow = area.removeFromTop(30);
    timeComboLabel.setBounds(timeRow.removeFromLeft(90));
    timeCombo.setBounds(timeRow.removeFromLeft(90).reduced(0, 3));
    analyzeButton.setBounds(timeRow.reduced(10, 2));

    area.removeFromTop(10);

    // 下段: 結果表示とApply
    auto resultRow = area.removeFromTop(70);
    auto labelsBox = resultRow.removeFromLeft(resultRow.getWidth() / 2);
    dryResultLabel.setBounds(labelsBox.removeFromTop(20));
    wetResultLabel.setBounds(labelsBox.removeFromTop(20));
    suggestResultLabel.setBounds(labelsBox.removeFromTop(25));

    applyButton.setBounds(resultRow.withSizeKeepingCentre(100, 30));

    // 最下段: Listen Bypass
    listenButton.setBounds(area.removeFromBottom(30).withSizeKeepingCentre(140, 24));
}