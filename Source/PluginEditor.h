#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI/Sections/InputSection.h"
#include "UI/Sections/DriveSection.h"
#include "UI/Sections/TransformerSection.h"
#include "UI/Sections/OutputSection.h"
#include "UI/Components/VerticalMeter.h"
#include "UI/Components/AnalyzerScreen.h"

class NeotoPreAudioProcessorEditor : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    NeotoPreAudioProcessorEditor(NeotoPreAudioProcessor&);
    ~NeotoPreAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    NeotoPreAudioProcessor& audioProcessor;

    // カスタムUIパーツ
    VerticalMeter inMeter;
    VerticalMeter outMeter;
    AnalyzerScreen analyzer;

    // 各コントロールセクション
    InputSection inputSec;
    DriveSection driveSec;
    TransformerSection transSec;
    OutputSection outputSec;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NeotoPreAudioProcessorEditor)
};