#include "PluginProcessor.h"
#include "PluginEditor.h"

NeotoPreAudioProcessorEditor::NeotoPreAudioProcessorEditor(NeotoPreAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p),
    inputSec(p), driveSec(p), transSec(p), outputSec(p) // AnalyzeSecは削除
{
    addAndMakeVisible(inMeter);
    addAndMakeVisible(outMeter);
    addAndMakeVisible(analyzer);

    addAndMakeVisible(inputSec);
    addAndMakeVisible(driveSec);
    addAndMakeVisible(transSec);
    addAndMakeVisible(outputSec);

    // 究極のプロトタイプサイズ: 横980 x 縦650
    setSize(980, 650);

    // メーター描画用に60FPSで回すタイマー
    startTimerHz(60);
}

NeotoPreAudioProcessorEditor::~NeotoPreAudioProcessorEditor()
{
    stopTimer();
}

void NeotoPreAudioProcessorEditor::timerCallback()
{
    // DSP側からPeak[dB]を取得し、メーターに渡す
    inMeter.setLevelDb(audioProcessor.inputPeakDb.load());
    outMeter.setLevelDb(audioProcessor.outputPeakDb.load());
}

void NeotoPreAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff181818)); // ダークで上質な背景色
}

void NeotoPreAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(10); // 全体の余白

    // ==============================================================================
    // 1. 両端のメーター (左右の領域を確保)
    // ==============================================================================
    inMeter.setBounds(area.removeFromLeft(30));

    area.removeFromLeft(15); // メーターと中央コンテンツの隙間

    outMeter.setBounds(area.removeFromRight(30));

    area.removeFromRight(15); // メーターと中央コンテンツの隙間

    // ==============================================================================
    // 2. 中央ブロックの上下分割 (アナライザー ＆ コントロール群)
    // ==============================================================================
    // 上段: アナライザー (高さを220pxに固定)
    analyzer.setBounds(area.removeFromTop(220));

    area.removeFromTop(15); // 上下段の隙間

    // 下段: 4つのコントロールセクション
    auto bottomTier = area;

    // 各セクションの横幅定義
    int standardWidth = 170;
    int spacing = 10;

    inputSec.setBounds(bottomTier.removeFromLeft(standardWidth));
    bottomTier.removeFromLeft(spacing);

    driveSec.setBounds(bottomTier.removeFromLeft(standardWidth));
    bottomTier.removeFromLeft(spacing);

    transSec.setBounds(bottomTier.removeFromLeft(standardWidth));
    bottomTier.removeFromLeft(spacing);

    // 残りの横幅すべて（約320px）をOutput(Gain Match統合)セクションに割り当て
    outputSec.setBounds(bottomTier);
}