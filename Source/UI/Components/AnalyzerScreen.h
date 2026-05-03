#pragma once
#include <JuceHeader.h>
#include "../../PluginProcessor.h"
#include "../../DSP/Algorithms/PreampModels.h"
#include "../../DSP/Transformers/TransformerModels.h"

class AnalyzerScreen : public juce::Component, public juce::Timer
{
public:
    AnalyzerScreen(NeotoPreAudioProcessor& p);
    ~AnalyzerScreen() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    NeotoPreAudioProcessor& audioProcessor;
    float getPositionForFrequency(float freq, float width);

    juce::dsp::FFT fft{ 11 };
    juce::dsp::WindowingFunction<float> window{ 2048, juce::dsp::WindowingFunction<float>::hann };

    std::array<float, 4096> eqFftData;
    juce::Path eqCurvePath;
    void generateEQCurve();

    std::array<float, 4096> spectrumFifo;
    std::array<float, 4096> spectrumFftData;
    std::array<float, 1024> smoothedSpectrum{ 0.0f }; // 追加: 波形スムージング用
    juce::Path spectrumPath;
    void drawNextFrameOfSpectrum();

    void calculateHarmonics(); // 追加: 倍音計算ロジック

    Preamp_API virtualPreamp_API;
    OutputTransformer_Steel virtualOut_Steel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AnalyzerScreen)
};