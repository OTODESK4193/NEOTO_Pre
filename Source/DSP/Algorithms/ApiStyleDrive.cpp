#include "ApiStyleDrive.h"
#include <cmath>
#include <algorithm>

void ApiStyleDrive::prepare(double sampleRate)
{
    fs = sampleRate;
    integratorState = 0.0;
    lastInputADAA = 0.0;
    lastSoftclipOut = 0.0;
    envState = 0.0;

    // エンベロープフォロワーの時定数設定 (Attack 5ms, Release 50ms)
    envAttackCoef = std::exp(-1.0 / (0.005 * fs));
    envReleaseCoef = std::exp(-1.0 / (0.050 * fs));

    lastDriveParam = -1.0f;
    lastCharParam = -1.0f;
    lastAsymParam = -1.0f;
    lastAgeParam = -1.0f;
}

double ApiStyleDrive::fx(double x) const { return x - (x * x * x) / 3.0; }
double ApiStyleDrive::F1(double x) const { double x2 = x * x; return (x2 / 2.0) - (x2 * x2 / 12.0); }

float ApiStyleDrive::processSample(float input, float driveParam, float charParam, float asymParam, float ageParam)
{
    // ==============================================================================
    // 1. パラメータのキャッシュ更新
    // ==============================================================================
    if (driveParam != lastDriveParam) {
        double driveDb = (static_cast<double>(driveParam) / 100.0) * 24.0;
        gain = juce::Decibels::decibelsToGain(driveDb);
        makeUp = 1.0 / (1.0 + (gain - 1.0) * 0.15);
        lastDriveParam = driveParam;
    }

    if (charParam != lastCharParam) {
        // Character: オペアンプの帯域幅（低域の膨らみの中心周波数）
        double fc = juce::jmap(static_cast<double>(charParam), 0.0, 100.0, 10.0, 60.0);
        alpha = std::exp(-juce::MathConstants<double>::twoPi * fc / fs);
        oneMinusAlpha = 1.0 - alpha;
        lastCharParam = charParam;
    }

    if (asymParam != lastAsymParam) {
        // Asymmetry: DCバイアスによる2次倍音の付加
        bias = juce::jmap(static_cast<double>(asymParam), 0.0, 100.0, 0.0, 0.4);
        fxBias = fx(bias);
        lastAsymParam = asymParam;
    }

    if (ageParam != lastAgeParam) {
        // Age: トランジェントによる電源電圧の降下（Sag）最大20%減衰
        sagRatio = juce::jmap(static_cast<double>(ageParam), 0.0, 100.0, 0.0, 0.2);
        lastAgeParam = ageParam;
    }

    // ==============================================================================
    // 2. DSPシグナルフロー
    // ==============================================================================
    double drivenSignal = static_cast<double>(input) * gain;

    // Sag (電源電圧の降下エミュレーション)
    double absIn = std::abs(drivenSignal);
    if (absIn > envState) envState = envAttackCoef * envState + (1.0 - envAttackCoef) * absIn;
    else envState = envReleaseCoef * envState + (1.0 - envReleaseCoef) * absIn;

    double dynamicGain = 1.0 - (envState * sagRatio);
    drivenSignal *= std::max(0.1, dynamicGain); // クリップ防止

    // Integrator
    double intOut = drivenSignal * oneMinusAlpha + alpha * integratorState;
    integratorState = intOut;

    // 非対称サチュレーション & 1次ADAA
    double x = std::clamp(intOut + bias, -1.0, 1.0);
    double softclipOut = 0.0;
    double dx = x - lastInputADAA;
    const double eps = 1e-8;

    if (std::abs(dx) < eps) softclipOut = fx((x + lastInputADAA) * 0.5);
    else softclipOut = (F1(x) - F1(lastInputADAA)) / dx;

    lastInputADAA = x;
    double outputWithoutDC = softclipOut - fxBias;

    // Differentiator キャンセル
    double diffOut = (outputWithoutDC - alpha * lastSoftclipOut) / oneMinusAlpha;
    lastSoftclipOut = outputWithoutDC;

    return static_cast<float>(diffOut * makeUp);
}