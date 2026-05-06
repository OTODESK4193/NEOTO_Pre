#include "OutputSection.h"

OutputSection::OutputSection(NeotoPreAudioProcessor& p) : audioProcessor(p)
{
    setupRotarySlider(outGainSlider, outGainLabel, "Output Gain");
    setupRotarySlider(mixSlider, mixLabel, "Mix (Dry/Wet)");

    outGainSlider.setLookAndFeel(&arcLnF);
    mixSlider.setLookAndFeel(&arcLnF);
    outGainSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::white);
    mixSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::white);

    listenButton.setClickingTogglesState(true);
    listenButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff222222));
    listenButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff00d4ff));
    listenButton.setButtonText("Listen");
    addAndMakeVisible(listenButton);

    timeCombo.addItemList({ "1 Second", "3 Seconds", "5 Seconds", "10 Seconds" }, 1);
    addAndMakeVisible(timeCombo);
    timeComboLabel.setText("Analyze Time", juce::dontSendNotification);
    timeComboLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(timeComboLabel);

    analyzeButton.setButtonText("Analyze");
    analyzeButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkcyan);
    analyzeButton.onClick = [this] {
        // ★ AudioThreadをブロックせず、GUIスレッドへのシグナルのみを発行
        audioProcessor.triggerAnalyze.store(true);
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
        lbl.setFont(juce::FontOptions(13.0f));
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

    outGainAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "output_gain", outGainSlider);
    mixAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "mix", mixSlider);
    listenAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(audioProcessor.apvts, "listen_mode", listenButton);
    timeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "analysis_time", timeCombo);

    startTimerHz(10);
}

OutputSection::~OutputSection() {
    stopTimer();
    outGainSlider.setLookAndFeel(nullptr);
    mixSlider.setLookAndFeel(nullptr);
}

void OutputSection::setupRotarySlider(juce::Slider& slider, juce::Label& label, const juce::String& name) {
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
    addAndMakeVisible(slider);
    label.setText(name, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(label);
}

void OutputSection::timerCallback() {
    // ★ ここで AudioThread の外 (Message Thread) で安全に解析処理を実行
    if (audioProcessor.triggerAnalyze.exchange(false)) {
        int timeSelection = static_cast<int>(audioProcessor.apvts.getRawParameterValue("analysis_time")->load());
        float seconds = (timeSelection == 0) ? 1.0f : (timeSelection == 1) ? 3.0f : (timeSelection == 2) ? 5.0f : 10.0f;
        audioProcessor.executeAnalyzer(seconds);
    }

    if (audioProcessor.hasNewAnalysisResult.exchange(false)) {
        auto& res = audioProcessor.latestAnalysisResult;
        dryResultLabel.setText(juce::String::formatted("Dry: %.1f LUFS", res.dryRmsL), juce::dontSendNotification);
        wetResultLabel.setText(juce::String::formatted("Wet: %.1f LUFS", res.wetRmsL), juce::dontSendNotification);
        juce::String sign = res.suggestedGainDb > 0.0f ? "+" : "";
        suggestResultLabel.setText("Suggest: " + sign + juce::String(res.suggestedGainDb, 1) + " dB", juce::dontSendNotification);
        suggestResultLabel.setColour(juce::Label::textColourId, juce::Colours::lightgreen);
    }
    repaint();
}

void OutputSection::paint(juce::Graphics& g) {
    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 8.0f);
    g.setColour(juce::Colours::grey);
    g.setFont(juce::FontOptions(15.0f));
    g.drawText("OUTPUT & MATCHING", getLocalBounds().removeFromTop(30), juce::Justification::centred, false);

    auto bounds = getLocalBounds().reduced(10);
    auto thdArea = bounds.withTrimmedTop(170);

    g.setFont(juce::FontOptions(10.0f));
    int numBars = 7;
    float barWidth = 14.0f;
    float spacing = (thdArea.getWidth() - (numBars * barWidth)) / (numBars - 1);
    float maxThd = 50.0f;

    for (int i = 0; i < numBars; ++i) {
        float val = audioProcessor.harmonicLevels[i].load();
        float normalized = juce::jlimit(0.0f, 1.0f, val / maxThd);
        float barHeight = normalized * (thdArea.getHeight() - 16.0f);

        float x = thdArea.getX() + i * (barWidth + spacing);
        float y = thdArea.getBottom() - 16.0f - barHeight;

        juce::Rectangle<float> barRect(x, y, barWidth, barHeight);

        if (i == 0) g.setColour(juce::Colours::white);
        else if (i % 2 != 0) g.setColour(juce::Colour(0xffffaa00));
        else g.setColour(juce::Colour(0xff00d4ff));

        g.fillRoundedRectangle(barRect, 2.0f);

        g.setColour(juce::Colours::grey);
        juce::String labelText = (i == 0) ? "1st" : (i == 1) ? "2nd" : (i == 2) ? "3rd" : juce::String(i + 1) + "th";
        g.drawText(labelText, x - 5.0f, thdArea.getBottom() - 14.0f, barWidth + 10.0f, 14.0f, juce::Justification::centredTop);
    }
}

void OutputSection::resized() {
    auto area = getLocalBounds().reduced(10);
    area.removeFromTop(20);

    auto topBlock = area.removeFromTop(95);

    auto outGainArea = topBlock.removeFromLeft(85);
    outGainLabel.setBounds(outGainArea.removeFromTop(20));
    outGainSlider.setBounds(outGainArea.withSizeKeepingCentre(75, 75));

    auto mixArea = topBlock.removeFromLeft(85);
    mixLabel.setBounds(mixArea.removeFromTop(20));
    mixSlider.setBounds(mixArea.withSizeKeepingCentre(75, 75));

    auto btnArea = topBlock.removeFromLeft(100).withTrimmedLeft(10);
    int btnHeight = 26;
    int btnSpacing = 8;
    listenButton.setBounds(btnArea.removeFromTop(btnHeight).withWidth(90));
    btnArea.removeFromTop(btnSpacing);
    analyzeButton.setBounds(btnArea.removeFromTop(btnHeight).withWidth(90));
    btnArea.removeFromTop(btnSpacing);
    applyButton.setBounds(btnArea.removeFromTop(btnHeight).withWidth(90));

    area.removeFromTop(5);

    auto timeRow = area.removeFromTop(24);
    timeComboLabel.setBounds(timeRow.removeFromLeft(90));
    timeCombo.setBounds(timeRow.removeFromLeft(100).reduced(0, 2));

    area.removeFromTop(5);

    auto labelRow = area.removeFromTop(20);
    int labelWidth = labelRow.getWidth() / 3;
    dryResultLabel.setBounds(labelRow.removeFromLeft(labelWidth));
    wetResultLabel.setBounds(labelRow.removeFromLeft(labelWidth));
    suggestResultLabel.setBounds(labelRow.removeFromLeft(labelWidth));
}