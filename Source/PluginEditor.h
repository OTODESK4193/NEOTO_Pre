#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

// 分割した5つのセクションをインクルード
#include "UI/Sections/InputSection.h"
#include "UI/Sections/DriveSection.h"
#include "UI/Sections/TransformerSection.h"
#include "UI/Sections/OutputSection.h"
#include "UI/Sections/AnalyzeSection.h"

//==============================================================================
class NeotoPreAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    NeotoPreAudioProcessorEditor(NeotoPreAudioProcessor&);
    ~NeotoPreAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    NeotoPreAudioProcessor& audioProcessor;

    // 独立した5つのUIコンポーネント（子クラス）
    InputSection inputSection;
    DriveSection driveSection;
    TransformerSection transformerSection;
    OutputSection outputSection;
    AnalyzeSection analyzeSection;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NeotoPreAudioProcessorEditor)
};