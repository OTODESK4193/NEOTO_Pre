#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
NeotoPreAudioProcessorEditor::NeotoPreAudioProcessorEditor(NeotoPreAudioProcessor& p)
    : AudioProcessorEditor(&p),
    audioProcessor(p),
    // 各子コンポーネントのコンストラクタへProcessorの参照を渡す
    inputSection(p),
    driveSection(p),
    transformerSection(p),
    outputSection(p),
    analyzeSection(p)
{
    // 子コンポーネントをエディターに追加して可視化
    addAndMakeVisible(inputSection);
    addAndMakeVisible(driveSection);
    addAndMakeVisible(transformerSection);
    addAndMakeVisible(outputSection);
    addAndMakeVisible(analyzeSection);

    // 5セクションを綺麗に配置するための幅1200px
    setSize(1200, 480);
}

NeotoPreAudioProcessorEditor::~NeotoPreAudioProcessorEditor()
{
}

//==============================================================================
void NeotoPreAudioProcessorEditor::paint(juce::Graphics& g)
{
    // 背景の塗りつぶし（全体のベースカラー）
    g.fillAll(juce::Colour(0xff1e1e1e));

    // ヘッダー領域の描画 (背景と共通タイトルのみ)
    auto headerArea = getLocalBounds().removeFromTop(45);
    g.setColour(juce::Colour(0xff111111));
    g.fillRect(headerArea);
    g.setColour(juce::Colours::white);
    g.setFont(22.0f);
    g.drawText("NEOTO Pre - Pro Studio", headerArea, juce::Justification::centred, true);

    // ※各セクションの背景パネルやタイトル（"INPUT STAGE"など）は、
    // それぞれのクラスの paint() メソッド内で自律的に描画されるため、ここには書きません。
}

void NeotoPreAudioProcessorEditor::resized()
{
    // ヘッダーを避けて、全体のコンテンツ領域を取得
    auto contentArea = getLocalBounds().withTrimmedTop(55).reduced(10);

    // 5等分した幅を計算
    int sectionWidth = contentArea.getWidth() / 5;

    // 左から順番に、各コンポーネントの領域を割り当てる (それぞれに5pxの余白を設ける)
    inputSection.setBounds(contentArea.removeFromLeft(sectionWidth).reduced(5));
    driveSection.setBounds(contentArea.removeFromLeft(sectionWidth).reduced(5));
    transformerSection.setBounds(contentArea.removeFromLeft(sectionWidth).reduced(5));
    outputSection.setBounds(contentArea.removeFromLeft(sectionWidth).reduced(5));

    // 残りの領域を最後のAnalyzeSectionへ割り当て
    analyzeSection.setBounds(contentArea.reduced(5));
}