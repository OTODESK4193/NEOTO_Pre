#include "PluginProcessor.h"
#include "PluginEditor.h"

NeotoPreAudioProcessorEditor::NeotoPreAudioProcessorEditor(NeotoPreAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p),
    analyzer(p), inputSec(p), driveSec(p), transSec(p), outputSec(p)
{
    addAndMakeVisible(inMeter);
    addAndMakeVisible(outMeter);
    addAndMakeVisible(analyzer);
    addAndMakeVisible(inputSec);
    addAndMakeVisible(driveSec);
    addAndMakeVisible(transSec);
    addAndMakeVisible(outputSec);

    setSize(980, 720);
    startTimerHz(60);
}

NeotoPreAudioProcessorEditor::~NeotoPreAudioProcessorEditor()
{
    stopTimer();
    // ★ ライフサイクル終了時の強制破棄処理: Ableton Live 11等でのクラッシュを防御
    removeAllChildren();
}

void NeotoPreAudioProcessorEditor::timerCallback()
{
    inMeter.setLevelDb(audioProcessor.inputPeakDb.load());
    outMeter.setLevelDb(audioProcessor.outputPeakDb.load());
}

void NeotoPreAudioProcessorEditor::paint(juce::Graphics& g)
{
    // グラデーション背景（木材ベースカラー）
    juce::ColourGradient gradient(
        juce::Colour(0xff2a1a0a),  // 濃い茶色（上）
        0.0f, 0.0f,
        juce::Colour(0xff5a3a28),  // 明るい茶色（下）
        0.0f, (float)getHeight(),
        false
    );
    g.setGradientFill(gradient);
    g.fillAll();

    // 横方向の木目パターン
    for (int y = 0; y < getHeight(); y++)
    {
        float smallGrain = sinf(y * 0.08f) * 0.3f;
        float mediumGrain = sinf(y * 0.02f) * 0.4f;
        float largeGrain = sinf(y * 0.005f) * 0.3f;
        float grainIntensity = smallGrain + mediumGrain + largeGrain;

        int alpha = (int)(15 + grainIntensity * 45);
        alpha = juce::jlimit(5, 60, alpha);

        g.setColour(juce::Colour(0, 0, 0).withAlpha(alpha / 255.0f));
        g.drawHorizontalLine(y, 0, getWidth());
    }

    // 縦方向の木目（立体感を追加）
    for (int x = 0; x < getWidth(); x += 30)
    {
        float verticalNoise = sinf(x * 0.01f) * 15.0f;
        g.setColour(juce::Colour(0, 0, 0).withAlpha(0.08f));
        g.drawVerticalLine(x + (int)verticalNoise, 0, getHeight());
    }
}

void NeotoPreAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(10);

    inMeter.setBounds(area.removeFromLeft(30));
    area.removeFromLeft(15);
    outMeter.setBounds(area.removeFromRight(30));
    area.removeFromRight(15);

    analyzer.setBounds(area.removeFromTop(360));
    area.removeFromTop(10);

    auto bottomTier = area;
    int standardWidth = 170;
    int spacing = 10;

    inputSec.setBounds(bottomTier.removeFromLeft(standardWidth));
    bottomTier.removeFromLeft(spacing);
    driveSec.setBounds(bottomTier.removeFromLeft(standardWidth));
    bottomTier.removeFromLeft(spacing);
    transSec.setBounds(bottomTier.removeFromLeft(standardWidth));
    bottomTier.removeFromLeft(spacing);

    outputSec.setBounds(bottomTier);
}