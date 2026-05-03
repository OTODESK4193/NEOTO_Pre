#include "TransformerModels.h"
#include <cmath>
#include <algorithm>

//==============================================================================
// Input: Nickel の実装 (旧 NickelTransformer.cpp と完全に同一)
//==============================================================================
void InputTransformer_Nickel::prepare(double sampleRate)
{
    fs = sampleRate;
    lastInputADAA = 0.0;
}

float InputTransformer_Nickel::processSample(float input)
{
    double dInput = static_cast<double>(input);
    double softclipOut = 0.0;
    double dx = dInput - lastInputADAA;
    const double eps = 1e-8;

    if (std::abs(dx) < eps) {
        softclipOut = ADAA_Math::fx_hardclip((dInput + lastInputADAA) * 0.5);
    }
    else {
        softclipOut = (ADAA_Math::F1_hardclip(dInput) - ADAA_Math::F1_hardclip(lastInputADAA)) / dx;
    }
    lastInputADAA = dInput;

    return static_cast<float>(softclipOut);
}

//==============================================================================
// Output: Steel の実装 (※後ほど既存のコードを上書きしてください)
//==============================================================================
void OutputTransformer_Steel::prepare(double sampleRate)
{
    fs = sampleRate;
}

float OutputTransformer_Steel::processSample(float input, float color, float air, float age)
{
    // 一旦コンパイルを通すためのバイパス処理
    return input;
}