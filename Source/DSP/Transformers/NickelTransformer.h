#pragma once
#include <JuceHeader.h>
// クラス内部の数学関数宣言を削除

class NickelTransformer
{
public:
    NickelTransformer() = default;
    ~NickelTransformer() = default;
    void prepare(double sampleRate);
    float processSample(float input);

private:
    double fs = 44100.0;
    double lastInputADAA = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NickelTransformer)
};