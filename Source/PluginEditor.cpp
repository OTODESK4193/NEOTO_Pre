#include "PluginProcessor.h"
#include "PluginEditor.h"

NeotoPreAudioProcessorEditor::NeotoPreAudioProcessorEditor(NeotoPreAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(1200, 380);

    // 1. Input Stage
    setupRotarySlider(inputGainSlider, inputGainLabel, "Input Gain");
    osModeCombo.addItemList({ "Off (1x)", "2x", "4x" }, 1);
    addAndMakeVisible(osModeCombo);
    osModeLabel.setText("Oversampling", juce::dontSendNotification);
    osModeLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(osModeLabel);

    // 2. Drive Stage
    setupRotarySlider(driveSlider, driveLabel, "Drive");
    setupRotarySlider(charSlider, charLabel, "Character");
    setupRotarySlider(asymSlider, asymLabel, "Asymmetry");

    // 3. Transformer Stage
    setupRotarySlider(colorSlider, colorLabel, "Color");
    setupRotarySlider(airSlider, airLabel, "Air");
    setupRotarySlider(ageSlider, ageLabel, "Age");

    // 4. Output Stage
    setupRotarySlider(outputGainSlider, outputGainLabel, "Output Gain");
    setupRotarySlider(mixSlider, mixLabel, "Mix (Dry/Wet)");

    listenButton.setButtonText("Listen: DRY (Bypass)");
    listenButton.setClickingTogglesState(true);
    listenButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::darkorange);
    addAndMakeVisible(listenButton);

    // 5. Auto-Level Stage (Gain Matcher)
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
        // 解析結果の推奨ゲインを「絶対値」として適用する（ループ加算バグの修正）
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

    // Attachments
    inputGainAttach = std::make_unique<SliderAttachment>(audioProcessor.apvts, "input_gain", inputGainSlider);
    outputGainAttach = std::make_unique<SliderAttachment>(audioProcessor.apvts, "output_gain", outputGainSlider);
    mixAttach = std::make_unique<SliderAttachment>(audioProcessor.apvts, "mix", mixSlider);
    osModeAttach = std::make_unique<ComboBoxAttachment>(audioProcessor.apvts, "os_mode", osModeCombo);
    listenAttach = std::make_unique<ButtonAttachment>(audioProcessor.apvts, "listen_mode", listenButton);
    driveAttach = std::make_unique<SliderAttachment>(audioProcessor.apvts, "drive", driveSlider);
    charAttach = std::make_unique<SliderAttachment>(audioProcessor.apvts, "character", charSlider);
    asymAttach = std::make_unique<SliderAttachment>(audioProcessor.apvts, "asymmetry", asymSlider);
    colorAttach = std::make_unique<SliderAttachment>(audioProcessor.apvts, "color", colorSlider);
    airAttach = std::make_unique<SliderAttachment>(audioProcessor.apvts, "air", airSlider);
    ageAttach = std::make_unique<SliderAttachment>(audioProcessor.apvts, "age", ageSlider);
    timeAttach = std::make_unique<ComboBoxAttachment>(audioProcessor.apvts, "analysis_time", timeCombo);

    startTimerHz(10);
}

NeotoPreAudioProcessorEditor::~NeotoPreAudioProcessorEditor()
{
    stopTimer();
}

void NeotoPreAudioProcessorEditor::setupRotarySlider(juce::Slider& slider, juce::Label& label, const juce::String& name)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(slider);
    label.setText(name, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(label);
}

void NeotoPreAudioProcessorEditor::timerCallback()
{
    if (audioProcessor.hasNewAnalysisResult.exchange(false))
    {
        auto& res = audioProcessor.latestAnalysisResult;

        float dryMax = std::max(res.dryRmsL, res.dryRmsR);
        float wetMax = std::max(res.wetRmsL, res.wetRmsR);

        float dryDb = dryMax > 0.0001f ? juce::Decibels::gainToDecibels(dryMax) : -100.0f;
        float wetDb = wetMax > 0.0001f ? juce::Decibels::gainToDecibels(wetMax) : -100.0f;

        dryResultLabel.setText(juce::String::formatted("Dry: %.1f LUFS", dryDb), juce::dontSendNotification);
        wetResultLabel.setText(juce::String::formatted("Wet: %.1f LUFS", wetDb), juce::dontSendNotification);

        // C++文字列演算子による安全な結合 (文字化けの修正)
        juce::String sign = res.suggestedGainDb > 0.0f ? "+" : "";
        juce::String suggestText = "Suggest: " + sign + juce::String(res.suggestedGainDb, 1) + " dB";

        suggestResultLabel.setText(suggestText, juce::dontSendNotification);
        suggestResultLabel.setColour(juce::Label::textColourId, juce::Colours::lightgreen);
    }
}

void NeotoPreAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1e1e1e));

    auto headerArea = getLocalBounds().removeFromTop(45);
    g.setColour(juce::Colour(0xff111111));
    g.fillRect(headerArea);
    g.setColour(juce::Colours::white);
    g.setFont(22.0f);
    g.drawText("NEOTO Pre - Pro Studio", headerArea, juce::Justification::centred, true);

    auto contentArea = getLocalBounds().withTrimmedTop(55).reduced(10);
    int sectionWidth = contentArea.getWidth() / 5;

    auto drawSection = [&](juce::Rectangle<int> area, const juce::String& title) {
        g.setColour(juce::Colour(0xff2a2a2a));
        g.fillRoundedRectangle(area.toFloat(), 8.0f);
        g.setColour(juce::Colours::grey);
        g.setFont(15.0f);
        g.drawText(title, area.removeFromTop(30), juce::Justification::centred, false);
        };

    drawSection(contentArea.removeFromLeft(sectionWidth).reduced(5), "INPUT STAGE");
    drawSection(contentArea.removeFromLeft(sectionWidth).reduced(5), "DRIVE STAGE");
    drawSection(contentArea.removeFromLeft(sectionWidth).reduced(5), "TRANSFORMER");
    drawSection(contentArea.removeFromLeft(sectionWidth).reduced(5), "OUTPUT STAGE");
    drawSection(contentArea.reduced(5), "GAIN MATCHING");
}

void NeotoPreAudioProcessorEditor::resized()
{
    auto contentArea = getLocalBounds().withTrimmedTop(55).reduced(10);
    int sectionWidth = contentArea.getWidth() / 5;

    auto layoutGrid = [&](juce::Rectangle<int> area,
        juce::Slider* s1, juce::Label* l1,
        juce::Slider* s2, juce::Label* l2,
        juce::Slider* s3, juce::Label* l3) {
            area.reduce(5, 5); area.removeFromTop(30);

            auto topRow = area.removeFromTop(area.getHeight() / 2);
            auto bottomRow = area;

            if (s1 && s2) {
                auto slot1 = topRow.removeFromLeft(topRow.getWidth() / 2);
                l1->setBounds(slot1.removeFromTop(20)); s1->setBounds(slot1.reduced(5));
                auto slot2 = topRow;
                l2->setBounds(slot2.removeFromTop(20)); s2->setBounds(slot2.reduced(5));
            }
            else if (s1) {
                l1->setBounds(topRow.removeFromTop(20)); s1->setBounds(topRow.reduced(5));
            }

            if (s3) {
                auto slot3 = bottomRow.withSizeKeepingCentre(100, bottomRow.getHeight());
                l3->setBounds(slot3.removeFromTop(20)); s3->setBounds(slot3.reduced(5));
            }
        };

    // 1. Input Stage
    auto inputArea = contentArea.removeFromLeft(sectionWidth);
    layoutGrid(inputArea, &inputGainSlider, &inputGainLabel, nullptr, nullptr, nullptr, nullptr);
    auto osArea = inputArea;
    osArea.reduce(5, 5); osArea.removeFromTop(inputArea.getHeight() / 2 + 30);
    osModeLabel.setBounds(osArea.removeFromTop(20).withSizeKeepingCentre(120, 20));
    osModeCombo.setBounds(osArea.removeFromTop(24).withSizeKeepingCentre(120, 24));

    // 2. Drive Stage
    auto driveArea = contentArea.removeFromLeft(sectionWidth);
    layoutGrid(driveArea, &driveSlider, &driveLabel, &charSlider, &charLabel, &asymSlider, &asymLabel);

    // 3. Transformer Stage
    auto transArea = contentArea.removeFromLeft(sectionWidth);
    layoutGrid(transArea, &colorSlider, &colorLabel, &airSlider, &airLabel, &ageSlider, &ageLabel);

    // 4. Output Stage
    auto outArea = contentArea.removeFromLeft(sectionWidth);
    layoutGrid(outArea, &outputGainSlider, &outputGainLabel, &mixSlider, &mixLabel, nullptr, nullptr);
    auto listenArea = outArea;
    listenArea.reduce(5, 5); listenArea.removeFromTop(outArea.getHeight() / 2 + 30);
    listenButton.setBounds(listenArea.withSizeKeepingCentre(160, 30));

    // 5. Gain Matching Stage (Target LUFS削除によるレイアウト調整)
    auto levelArea = contentArea;
    levelArea.reduce(5, 5); levelArea.removeFromTop(30);

    auto topLevel = levelArea.removeFromTop(60);
    timeComboLabel.setBounds(topLevel.removeFromTop(15).withSizeKeepingCentre(100, 15));
    timeCombo.setBounds(topLevel.removeFromTop(24).withSizeKeepingCentre(100, 24));

    auto midLevel = levelArea.removeFromTop(40);
    analyzeButton.setBounds(midLevel.withSizeKeepingCentre(120, 30));

    auto bottomLevel = levelArea;
    dryResultLabel.setBounds(bottomLevel.removeFromTop(20));
    wetResultLabel.setBounds(bottomLevel.removeFromTop(20));
    suggestResultLabel.setBounds(bottomLevel.removeFromTop(25));
    applyButton.setBounds(bottomLevel.removeFromTop(35).withSizeKeepingCentre(140, 30));
}