#pragma once
#include <JuceHeader.h>

class SteelTransformer
{
public:
    SteelTransformer() = default;
    ~SteelTransformer() = default;
    void prepare(double sampleRate);
    float processSample(float input, float colorParam, float airParam, float ageParam);

private:
    double fs = 44100.0;

    // フィルタ状態変数
    double hpfState = 0.0;
    double lastInput = 0.0;
    double apfState = 0.0;
    double lastApfInput = 0.0;
    double lpfState = 0.0; // Age用 高域減衰

    // Biquad (Air EQ) 状態変数
    double x1 = 0.0, x2 = 0.0, y1 = 0.0, y2 = 0.0;
    double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;

    // キャッシュ変数
    float lastColorParam = -1.0f;
    float lastAirParam = -1.0f;
    float lastAgeParam = -1.0f;

    double alphaHpf = 0.0;
    double apfAlpha = 0.0;
    double alphaLpf = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SteelTransformer)
};