#pragma once

#include <JuceHeader.h>
#include "../../PluginProcessor.h"
#include "../../DSP/Transformers/TransformerModels.h"

// ※プリアンプモデルのヘッダファイルへのパスは、実際の環境に合わせて調整してください
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
    float getPositionForFrequency(float freq, float width);

    NeotoPreAudioProcessor& audioProcessor;

    // ==============================================================================
    // FFT および描画用のバッファとインスタンス
    // ==============================================================================
    juce::dsp::FFT fft{ 13 }; // 8192 points (2^13)
    juce::dsp::WindowingFunction<float> window{ 8192, juce::dsp::WindowingFunction<float>::hann };

    std::array<float, 8192> circularBuffer{ 0.0f };
    std::array<float, 16384> spectrumFftData{ 0.0f };
    std::array<float, 8192> smoothedSpectrum{ 0.0f };
    std::array<float, 16384> eqFftData{ 0.0f };

    juce::Path spectrumPath;
    juce::Path eqCurvePath;

    // ==============================================================================
    // ★ アナライザー描画用の仮想DSPエンジン群
    // ==============================================================================
    InputTransformer_Nickel    virtualIn_Nickel;
    InputTransformer_Steel     virtualIn_Steel;
    InputTransformer_Iron      virtualIn_Iron;
    InputTransformer_Amorphous virtualIn_Amorphous;

    // ★ 変数名を CPP 側と統一しました
    Preamp_API virtualPreamp_API;

    OutputTransformer_Nickel    virtualOut_Nickel;
    OutputTransformer_Steel     virtualOut_Steel;
    OutputTransformer_Iron      virtualOut_Iron;
    OutputTransformer_Amorphous virtualOut_Amorphous;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AnalyzerScreen)
};