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
        double fc = juce::jmap(static_cast<double>(charParam), 0.0, 100.0, 10.0, 60.0);
        alpha = std::exp(-juce::MathConstants<double>::twoPi * fc / fs);
        oneMinusAlpha = 1.0 - alpha;
        lastCharParam = charParam;
    }

    if (asymParam != lastAsymParam) {
        bias = juce::jmap(static_cast<double>(asymParam), 0.0, 100.0, 0.0, 0.4);
        fxBias = ADAA_Math::fx_cubic(bias);
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

    double intOut = drivenSignal * oneMinusAlpha + alpha * integratorState;
    integratorState = intOut;

    // 数学的に連続な区分定義積分 F1_cubic 内でハードクリップを処理することで、積分破壊を解決する。
    double x = intOut + bias;

    double softclipOut = 0.0;
    double dx = x - lastInputADAA;
    double abs_dx = std::abs(dx);

    // ★ 変更: デュアルスレッショルドによる適応的クロスフェードの導入
    const double eps_lower = 1e-8; // 完全にフォールバックする下限
    const double eps_upper = 1e-4; // クロスフェードを開始する上限

    if (abs_dx < eps_lower) {
        // 100% テイラー展開による近似 (ゼロ除算回避)
        softclipOut = ADAA_Math::fx_cubic((x + lastInputADAA) * 0.5);
    }
    else if (abs_dx < eps_upper) {
        // 遷移領域: 滑らかなクロスフェード (Adaptive Crossfade)
        double fallback = ADAA_Math::fx_cubic((x + lastInputADAA) * 0.5);
        double adaa = (ADAA_Math::F1_cubic(x) - ADAA_Math::F1_cubic(lastInputADAA)) / dx;

        // 距離に応じた重み付け (0.0 〜 1.0)
        double w = (abs_dx - eps_lower) / (eps_upper - eps_lower);
        softclipOut = w * adaa + (1.0 - w) * fallback;
    }
    else {
        // 100% 正確な積分差分 (1次ADAA)
        softclipOut = (ADAA_Math::F1_cubic(x) - ADAA_Math::F1_cubic(lastInputADAA)) / dx;
    }

    lastInputADAA = x;
    double outputWithoutDC = softclipOut - fxBias;

    double diffOut = (outputWithoutDC - alpha * lastSoftclipOut) / oneMinusAlpha;
    lastSoftclipOut = outputWithoutDC;

    return static_cast<float>(diffOut * makeUp);
}