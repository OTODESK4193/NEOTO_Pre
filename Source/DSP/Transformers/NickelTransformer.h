#pragma once
#include <JuceHeader.h>

class NickelTransformer
{
public:
    NickelTransformer() = default;
    ~NickelTransformer() = default;

    void prepare(double sampleRate);
    float processSample(float input);

private:
    // ハードクリッピング（tanh近似等）と1次ADAA用の関数
    float fx(float x) const;
    float F1(float x) const;

    double fs = 44100.0;
    float lastInputADAA = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NickelTransformer)
};