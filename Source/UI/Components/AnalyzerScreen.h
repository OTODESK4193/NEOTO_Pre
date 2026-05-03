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

    // ★ FFTエンジンを 8192ポイント(13) に倍増し、超高解像度化
    juce::dsp::FFT fft{ 13 };
    juce::dsp::WindowingFunction<float> window{ 8192, juce::dsp::WindowingFunction<float>::hann };

    std::array<float, 16384> eqFftData;
    juce::Path eqCurvePath;
    void generateEQCurve();

    // ★ 滑らかな描画のためのスライディング・バッファ
    std::array<float, 8192> circularBuffer{ 0.0f };
    std::array<float, 16384> spectrumFftData;
    std::array<float, 4096> smoothedSpectrum{ 0.0f };
    juce::Path spectrumPath;
    void drawNextFrameOfSpectrum();

    void calculateHarmonics();

    Preamp_API virtualPreamp_API;
    OutputTransformer_Steel virtualOut_Steel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AnalyzerScreen)
};