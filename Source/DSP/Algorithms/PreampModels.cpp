#include "PreampModels.h"
#include <cmath>
#include <algorithm>

//==============================================================================
// API Style の実装
//==============================================================================
void Preamp_API::prepare(double sampleRate)
{
    fs = sampleRate;
    integratorState = 0.0;
    lastInputADAA = 0.0;
    lastSoftclipOut = 0.0;
    envState = 0.0;

    envAttackCoef = std::exp(-1.0 / (0.005 * fs));
    envReleaseCoef = std::exp(-1.0 / (0.050 * fs));

    lastDriveParam = -1.0f;
    lastCharParam = -1.0f;
    lastAsymParam = -1.0f;
    lastAgeParam = -1.0f;
}

float Preamp_API::processSample(float input, float driveParam, float charParam, float asymParam, float ageParam)
{
    if (driveParam != lastDriveParam) {
        double driveDb = (static_cast<double>(driveParam) / 100.0) * 24.0;
        gain = juce::Decibels::decibelsToGain(driveDb);
        makeUp = 1.0 / (1.0 + (gain - 1.0) * 0.15);
        lastDriveParam = driveParam;
    }

    if (charParam != lastCharParam) {
        // Character パラメータの役割を「倍音バランス」の制御へ再定義
        // 0.0 = Odd (3次倍音主体), 100.0 = Even (2次倍音主体)
        double charNormalized = static_cast<double>(charParam) / 100.0;
        mixEven = charNormalized;
        mixOdd = 1.0 - charNormalized;

        // 積分器(Integrator)のカットオフは固定化、あるいは微調整に留める
        double fc = 35.0; // 中庸な設定で固定
        alpha = std::exp(-juce::MathConstants<double>::twoPi * fc / fs);
        oneMinusAlpha = 1.0 - alpha;
        lastCharParam = charParam;
    }

    if (asymParam != lastAsymParam) {
        // Asymmetryノブは、従来通りDCバイアス（さらなる非対称性と劣化の付加）として機能
        bias = juce::jmap(static_cast<double>(asymParam), 0.0, 100.0, 0.0, 0.4);

        // バイアス加算時の直流オフセットを相殺するための補正値
        fxBias = (ADAA_Math::fx_chebyshev_even(bias) * mixEven) + (ADAA_Math::fx_chebyshev_odd(bias) * mixOdd);
        lastAsymParam = asymParam;
    }

    if (ageParam != lastAgeParam) {
        sagRatio = juce::jmap(static_cast<double>(ageParam), 0.0, 100.0, 0.0, 0.2);
        lastAgeParam = ageParam;
    }

    double drivenSignal = static_cast<double>(input) * gain;
    double absIn = std::abs(drivenSignal);

    if (absIn > envState) envState = envAttackCoef * envState + (1.0 - envAttackCoef) * absIn;
    else envState = envReleaseCoef * envState + (1.0 - envReleaseCoef) * absIn;

    double dynamicGain = 1.0 - (envState * sagRatio);
    drivenSignal *= std::max(0.1, dynamicGain);

    // Integrator
    double intOut = drivenSignal * oneMinusAlpha + alpha * integratorState;
    integratorState = intOut;

    double x = intOut + bias;

    double softclipOut = 0.0;
    double dx = x - lastInputADAA;
    double abs_dx = std::abs(dx);

    // デュアルスレッショルドによる適応的クロスフェード
    const double eps_lower = 1e-8;
    const double eps_upper = 1e-4;

    // ★ 変更: ラムダ式の戻り値型を -> double と明示し、変数のキャプチャを確実に行う
    auto compute_fallback = [&](double val) -> double {
        return (ADAA_Math::fx_chebyshev_even(val) * mixEven) + (ADAA_Math::fx_chebyshev_odd(val) * mixOdd);
        };

    auto compute_adaa = [&](double val_cur, double val_prev, double delta) -> double {
        double f1_even_cur = ADAA_Math::F1_chebyshev_even(val_cur);
        double f1_even_prev = ADAA_Math::F1_chebyshev_even(val_prev);
        double adaa_even = (f1_even_cur - f1_even_prev) / delta;

        double f1_odd_cur = ADAA_Math::F1_chebyshev_odd(val_cur);
        double f1_odd_prev = ADAA_Math::F1_chebyshev_odd(val_prev);
        double adaa_odd = (f1_odd_cur - f1_odd_prev) / delta;

        return (adaa_even * mixEven) + (adaa_odd * mixOdd);
        };

    if (abs_dx < eps_lower) {
        softclipOut = compute_fallback((x + lastInputADAA) * 0.5);
    }
    else if (abs_dx < eps_upper) {
        double fallback = compute_fallback((x + lastInputADAA) * 0.5);
        double adaa = compute_adaa(x, lastInputADAA, dx);
        double w = (abs_dx - eps_lower) / (eps_upper - eps_lower);
        softclipOut = w * adaa + (1.0 - w) * fallback;
    }
    else {
        softclipOut = compute_adaa(x, lastInputADAA, dx);
    }

    lastInputADAA = x;
    double outputWithoutDC = softclipOut - fxBias;

    // Differentiator
    double diffOut = (outputWithoutDC - alpha * lastSoftclipOut) / oneMinusAlpha;
    lastSoftclipOut = outputWithoutDC;

    return static_cast<float>(diffOut * makeUp);
}