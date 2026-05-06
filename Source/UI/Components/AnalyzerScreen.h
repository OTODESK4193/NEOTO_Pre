#pragma once
#include <JuceHeader.h>
#include "../../PluginProcessor.h"
#include "../../DSP/Transformers/TransformerModels.h"
#include "../../DSP/Algorithms/PreampModels.h"

class AnalyzerScreen : public juce::Component, public juce::Timer
{
public:
    AnalyzerScreen(NeotoPreAudioProcessor&);
    ~AnalyzerScreen() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    void drawNextFrameOfSpectrum();
    void generateEQCurve();
    void calculateHarmonics();

    // 座標変換関数
    float getPositionForFrequency(float freq, float width);
    float getFrequencyForPosition(float x, float width);

    NeotoPreAudioProcessor& audioProcessor;

    juce::dsp::FFT fft{ 13 };
    juce::dsp::WindowingFunction<float> window{ 8192, juce::dsp::WindowingFunction<float>::hann };

    std::array<float, 8192> circularBuffer{ 0.0f };
    std::array<float, 16384> spectrumFftData{ 0.0f };
    std::array<float, 8192> smoothedSpectrum{ 0.0f };
    std::array<float, 16384> eqFftData{ 0.0f };

    juce::Path spectrumPath;
    juce::Path eqCurvePath;

    // 入力トランスの仮想インスタンス
    InputTransformer_Nickel    virtualIn_Nickel;
    InputTransformer_Steel     virtualIn_Steel;
    InputTransformer_Iron      virtualIn_Iron;
    InputTransformer_Amorphous virtualIn_Amorphous;
    InputTransformer_Carnhill  virtualIn_Carnhill; // ★追加
    InputTransformer_Cinemag   virtualIn_Cinemag;  // ★追加

    // プリアンプの仮想インスタンス
    Preamp_API virtualPreamp_API;
    Preamp_Neve virtualPreamp_Neve;
    Preamp_Tube virtualPreamp_Tube;
    Preamp_SSL virtualPreamp_SSL;
    Preamp_Modern1 virtualPreamp_Modern1;
    Preamp_Modern2 virtualPreamp_Modern2;

    // 出力トランスの仮想インスタンス
    OutputTransformer_Nickel    virtualOut_Nickel;
    OutputTransformer_Steel     virtualOut_Steel;
    OutputTransformer_Iron      virtualOut_Iron;
    OutputTransformer_Amorphous virtualOut_Amorphous;
    OutputTransformer_Carnhill  virtualOut_Carnhill; // ★追加
    OutputTransformer_Cinemag   virtualOut_Cinemag;  // ★追加

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AnalyzerScreen)
};