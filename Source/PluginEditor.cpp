#include "PluginProcessor.h"
#include "PluginEditor.h"

NeotoPreAudioProcessorEditor::NeotoPreAudioProcessorEditor(NeotoPreAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p),
    analyzer(p), inputSec(p), driveSec(p), transSec(p), outputSec(p) // ★ analyzer(p) を追加
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

NeotoPreAudioProcessorEditor::~NeotoPreAudioProcessorEditor() { stopTimer(); }

void NeotoPreAudioProcessorEditor::timerCallback()
{
    inMeter.setLevelDb(audioProcessor.inputPeakDb.load());
    outMeter.setLevelDb(audioProcessor.outputPeakDb.load());
}

void NeotoPreAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff181818));
}

void NeotoPreAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(10);

    inMeter.setBounds(area.removeFromLeft(30));
    area.removeFromLeft(15);
    outMeter.setBounds(area.removeFromRight(30));
    area.removeFromRight(15);

    // ★ アナライザーの高さを 360px まで大拡張 (余白を詰める)
    analyzer.setBounds(area.removeFromTop(360));
    area.removeFromTop(10); // 隙間を15pxから10pxへ圧縮

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