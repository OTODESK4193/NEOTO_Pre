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

    // ★ FFTエンジンを 4096ポイント(12) に倍増し、低域の解像度を向上
    juce::dsp::FFT fft{ 12 };
    juce::dsp::WindowingFunction<float> window{ 4096, juce::dsp::WindowingFunction<float>::hann };

    std::array<float, 8192> eqFftData;
    juce::Path eqCurvePath;
    void generateEQCurve();

    std::array<float, 8192> spectrumFifo;
    std::array<float, 8192> spectrumFftData;
    std::array<float, 2048> smoothedSpectrum{ 0.0f }; // 半分のNyquistまで
    juce::Path spectrumPath;
    void drawNextFrameOfSpectrum();

    void calculateHarmonics();

    Preamp_API virtualPreamp_API;
    OutputTransformer_Steel virtualOut_Steel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AnalyzerScreen)
};