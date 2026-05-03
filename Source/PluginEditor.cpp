#include "PluginProcessor.h"
#include "PluginEditor.h"

NeotoPreAudioProcessorEditor::NeotoPreAudioProcessorEditor(NeotoPreAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(800, 350);

    // 1. Drive & 2. Transformer
    setupRotarySlider(driveSlider, driveLabel, "Drive");
    setupRotarySlider(charSlider, charLabel, "Character");
    setupRotarySlider(asymSlider, asymLabel, "Asymmetry");
    setupRotarySlider(colorSlider, colorLabel, "Color");
    setupRotarySlider(airSlider, airLabel, "Air");
    setupRotarySlider(ageSlider, ageLabel, "Age");

    // 3. Auto-Level
    setupRotarySlider(targetLufsSlider, targetLufsLabel, "Target LUFS");

    // コンボボックスの設定
    timeCombo.addItemList({ "1 Second", "3 Seconds", "5 Seconds", "10 Seconds" }, 1);
    addAndMakeVisible(timeCombo);

    timeComboLabel.setText("Analysis Time", juce::dontSendNotification);
    timeComboLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(timeComboLabel);

    // Applyボタンの設定
    applyButton.setButtonText("Apply Level");
    applyButton.onClick = [this] {
        // ボタンが押されたら、Processor側のフラグを立てて計算を要求する
        audioProcessor.triggerAutoLevel = true;
        };
    addAndMakeVisible(applyButton);

    // アタッチメント
    driveAttach = std::make_unique<SliderAttachment>(audioProcessor.apvts, "drive", driveSlider);
    charAttach = std::make_unique<SliderAttachment>(audioProcessor.apvts, "character", charSlider);
    asymAttach = std::make_unique<SliderAttachment>(audioProcessor.apvts, "asymmetry", asymSlider);
    colorAttach = std::make_unique<SliderAttachment>(audioProcessor.apvts, "color", colorSlider);
    airAttach = std::make_unique<SliderAttachment>(audioProcessor.apvts, "air", airSlider);
    ageAttach = std::make_unique<SliderAttachment>(audioProcessor.apvts, "age", ageSlider);
    targetLufsAttach = std::make_unique<SliderAttachment>(audioProcessor.apvts, "autolevel_target", targetLufsSlider);
    timeAttach = std::make_unique<ComboBoxAttachment>(audioProcessor.apvts, "analysis_time", timeCombo);
}

NeotoPreAudioProcessorEditor::~NeotoPreAudioProcessorEditor() {}

void NeotoPreAudioProcessorEditor::setupRotarySlider(juce::Slider& slider, juce::Label& label, const juce::String& name)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(slider);

    label.setText(name, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(label);
}

void NeotoPreAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff222222));

    auto headerArea = getLocalBounds().removeFromTop(40);
    g.setColour(juce::Colour(0xff111111));
    g.fillRect(headerArea);
    g.setColour(juce::Colours::white);
    g.setFont(20.0f);
    g.drawText("NEOTO Pre - Pro Mode", headerArea, juce::Justification::centred, true);

    auto contentArea = getLocalBounds().withTrimmedTop(50).reduced(10);
    int sectionWidth = contentArea.getWidth() / 3;

    auto drawSection = [&](juce::Rectangle<int> area, const juce::String& title) {
        g.setColour(juce::Colour(0xff2a2a2a));
        g.fillRoundedRectangle(area.toFloat(), 8.0f);
        g.setColour(juce::Colours::grey);
        g.setFont(16.0f);
        g.drawText(title, area.removeFromTop(30), juce::Justification::centred, false);
        };

    drawSection(contentArea.removeFromLeft(sectionWidth).reduced(5), "DRIVE STAGE");
    drawSection(contentArea.removeFromLeft(sectionWidth).reduced(5), "TRANSFORMER");
    drawSection(contentArea.reduced(5), "AUTO-LEVEL");
}

void NeotoPreAudioProcessorEditor::resized()
{
    auto contentArea = getLocalBounds().withTrimmedTop(50).reduced(10);
    int sectionWidth = contentArea.getWidth() / 3;

    auto layoutSection = [&](juce::Rectangle<int> area, juce::Slider& s1, juce::Label& l1,
        juce::Slider& s2, juce::Label& l2,
        juce::Slider& s3, juce::Label& l3) {
            area.reduce(5, 5);
            area.removeFromTop(30);

            int itemWidth = area.getWidth() / 3;

            auto slot1 = area.removeFromLeft(itemWidth);
            l1.setBounds(slot1.removeFromTop(20));
            s1.setBounds(slot1.reduced(5));

            auto slot2 = area.removeFromLeft(itemWidth);
            l2.setBounds(slot2.removeFromTop(20));
            s2.setBounds(slot2.reduced(5));

            auto slot3 = area;
            l3.setBounds(slot3.removeFromTop(20));
            s3.setBounds(slot3.reduced(5));
        };

    auto driveArea = contentArea.removeFromLeft(sectionWidth);
    layoutSection(driveArea, driveSlider, driveLabel, charSlider, charLabel, asymSlider, asymLabel);

    auto transArea = contentArea.removeFromLeft(sectionWidth);
    layoutSection(transArea, colorSlider, colorLabel, airSlider, airLabel, ageSlider, ageLabel);

    // Auto-Level Stage の専用レイアウト
    auto levelArea = contentArea;
    levelArea.reduce(5, 5);
    levelArea.removeFromTop(30); // タイトル分のスペース

    // コンボボックスとApplyボタンを上部に配置
    auto topArea = levelArea.removeFromTop(50);
    timeComboLabel.setBounds(topArea.removeFromLeft(80).withSizeKeepingCentre(80, 20));
    timeCombo.setBounds(topArea.removeFromLeft(90).withSizeKeepingCentre(80, 24));

    // 少し余白を空けてApplyボタン
    topArea.removeFromLeft(10);
    applyButton.setBounds(topArea.withSizeKeepingCentre(100, 30));

    levelArea.removeFromTop(10); // 余白

    // Target LUFSスライダーを下部に配置
    targetLufsLabel.setBounds(levelArea.removeFromTop(20).withSizeKeepingCentre(100, 20));
    targetLufsSlider.setBounds(levelArea.reduced(20, 0));
}