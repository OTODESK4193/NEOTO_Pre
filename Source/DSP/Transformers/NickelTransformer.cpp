#include "NickelTransformer.h"
#include "../Core/ADAA_Math.h" // 新規作成したライブラリをインクルード
#include <cmath>
#include <algorithm>

void NickelTransformer::prepare(double sampleRate)
{
    fs = sampleRate;
    lastInputADAA = 0.0;
}

float NickelTransformer::processSample(float input)
{
    double dInput = static_cast<double>(input);
    double softclipOut = 0.0;
    double dx = dInput - lastInputADAA;
    const double eps = 1e-8;

    if (std::abs(dx) < eps) {
        // 共通ライブラリを呼び出し
        softclipOut = ADAA_Math::fx_hardclip((dInput + lastInputADAA) * 0.5);
    }
    else {
        // 共通ライブラリを呼び出し
        softclipOut = (ADAA_Math::F1_hardclip(dInput) - ADAA_Math::F1_hardclip(lastInputADAA)) / dx;
    }
    lastInputADAA = dInput;

    return static_cast<float>(softclipOut);
}