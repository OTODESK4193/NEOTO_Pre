#include "TransformerModels.h"
#include <cmath>
#include <algorithm>

//==============================================================================
// Input: Nickel の実装
//==============================================================================
void InputTransformer_Nickel::prepare(double sampleRate) { fs = sampleRate; lastInputADAA = 0.0; }
float InputTransformer_Nickel::processSample(float input) {
    double dInput = static_cast<double>(input); double dx = dInput - lastInputADAA; double softclipOut = 0.0;
    if (std::abs(dx) < 1e-8) softclipOut = ADAA_Math::fx_hardclip((dInput + lastInputADAA) * 0.5);
    else softclipOut = (ADAA_Math::F1_hardclip(dInput) - ADAA_Math::F1_hardclip(lastInputADAA)) / dx;
    lastInputADAA = dInput; return static_cast<float>(softclipOut);
}

//==============================================================================
// Input: Steel の実装
//==============================================================================
void InputTransformer_Steel::prepare(double sampleRate) {
    fs = sampleRate; lastInputADAA = 0.0; hpfState = 0.0; lastInput = 0.0;
    alphaHpf = std::exp(-juce::MathConstants<double>::twoPi * 15.0 / fs);
}
float InputTransformer_Steel::processSample(float input) {
    double dInput = static_cast<double>(input);
    double hpfOut = dInput - lastInput + alphaHpf * hpfState;
    hpfState = hpfOut; lastInput = dInput;
    double softclipOut = 0.0; double dx = hpfOut - lastInputADAA;
    if (std::abs(dx) < 1e-8) softclipOut = ADAA_Math::fx_steel((hpfOut + lastInputADAA) * 0.5);
    else softclipOut = (ADAA_Math::F1_steel(hpfOut) - ADAA_Math::F1_steel(lastInputADAA)) / dx;
    lastInputADAA = hpfOut; return static_cast<float>(softclipOut);
}

//==============================================================================
// Input: Iron の実装
//==============================================================================
void InputTransformer_Iron::prepare(double sampleRate) { fs = sampleRate; lastInputADAA = 0.0; }
float InputTransformer_Iron::processSample(float input) {
    double dInput = static_cast<double>(input); double dx = dInput - lastInputADAA; double softclipOut = 0.0;
    if (std::abs(dx) < 1e-8) softclipOut = ADAA_Math::fx_iron((dInput + lastInputADAA) * 0.5);
    else softclipOut = (ADAA_Math::F1_iron(dInput) - ADAA_Math::F1_iron(lastInputADAA)) / dx;
    lastInputADAA = dInput; return static_cast<float>(softclipOut);
}

//==============================================================================
// Input: Amorphous の実装
//==============================================================================
void InputTransformer_Amorphous::prepare(double sampleRate) { fs = sampleRate; lastInputADAA = 0.0; }
float InputTransformer_Amorphous::processSample(float input) {
    double dInput = static_cast<double>(input); double dx = dInput - lastInputADAA; double softclipOut = 0.0;
    auto fx_amorph = [](double x) { return std::clamp(x, -4.0, 4.0); };
    auto F1_amorph = [](double x) {
        if (x > 4.0) return 4.0 * x - 8.0; if (x < -4.0) return -4.0 * x - 8.0; return 0.5 * x * x;
        };
    if (std::abs(dx) < 1e-8) softclipOut = fx_amorph((dInput + lastInputADAA) * 0.5);
    else softclipOut = (F1_amorph(dInput) - F1_amorph(lastInputADAA)) / dx;
    lastInputADAA = dInput; return static_cast<float>(softclipOut);
}

//==============================================================================
// Output: Steel の実装 (Tellinen: ワイドなヒステリシスとフェイズシフト)
//==============================================================================
void OutputTransformer_Steel::prepare(double sampleRate) {
    fs = sampleRate; hpfState = 0.0; lastInput = 0.0; apfState = 0.0; lastApfInput = 0.0; lpfState = 0.0;
    x1 = 0.0; x2 = 0.0; y1 = 0.0; y2 = 0.0;
    lastColorParam = -1.0f; lastAirParam = -1.0f; lastAgeParam = -1.0f;
    hysteresisEngine.prepare();
}

float OutputTransformer_Steel::processSample(float input, float colorParam, float airParam, float ageParam) {
    if (colorParam != lastColorParam) {
        double fcHpf = juce::jmap(static_cast<double>(colorParam), 0.0, 100.0, 15.0, 5.0);
        alphaHpf = std::exp(-juce::MathConstants<double>::twoPi * fcHpf / fs);

        double apfFreq = juce::jmap(static_cast<double>(colorParam), 0.0, 100.0, 60.0, 20.0);
        double tanVal = std::tan(juce::MathConstants<double>::pi * apfFreq / fs);
        apfAlpha = (tanVal - 1.0) / (tanVal + 1.0);

        hystHc = juce::jmap(static_cast<double>(colorParam), 0.0, 100.0, 0.05, 0.30);
        hystDrive = juce::jmap(static_cast<double>(colorParam), 0.0, 100.0, 1.2, 2.5);
        lastColorParam = colorParam;
    }

    if (airParam != lastAirParam) {
        double gainDb = juce::jmap(static_cast<double>(airParam), 0.0, 100.0, 0.0, 6.0);
        double A = std::pow(10.0, gainDb / 40.0);
        double w0 = juce::MathConstants<double>::twoPi * 15000.0 / fs;
        double alphaQ = std::sin(w0) / (2.0 * 0.707);
        double a0 = 1.0 + alphaQ / A;
        b0 = (1.0 + alphaQ * A) / a0; b1 = (-2.0 * std::cos(w0)) / a0; b2 = (1.0 - alphaQ * A) / a0;
        a1 = (-2.0 * std::cos(w0)) / a0; a2 = (1.0 - alphaQ / A) / a0;
        lastAirParam = airParam;
    }

    if (ageParam != lastAgeParam) {
        double fcLpf = juce::jmap(static_cast<double>(ageParam), 0.0, 100.0, 30000.0, 10000.0);
        alphaLpf = std::exp(-juce::MathConstants<double>::twoPi * fcLpf / fs);
        lastAgeParam = ageParam;
    }

    double dIn = static_cast<double>(input);

    // 位相シフト(APF)を最初にかける
    double apfOut = apfAlpha * dIn + lastApfInput - apfAlpha * apfState;
    apfState = apfOut; lastApfInput = dIn;

    // 変更前: double hystOut = hysteresisEngine.processSample(...)
        // 変更後↓
    double hystOut = isAnalyzerMode ? static_cast<double>(apfOut)
        : hysteresisEngine.processSample(static_cast<float>(apfOut), hystDrive, hystHc);

    // Air Biquad
    double airOut = b0 * hystOut + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    x2 = x1; x1 = hystOut; y2 = y1; y1 = airOut;

    // Age LPF
    double lpfOut = airOut * (1.0 - alphaLpf) + alphaLpf * lpfState;
    lpfState = lpfOut;

    // ★ 物理モデルの最終段にHPFを移動し、残留磁化（DCオフセット）を完全遮断
    double hpfOut = lpfOut - lastInput + alphaHpf * hpfState;
    hpfState = hpfOut; lastInput = lpfOut;

    return static_cast<float>(hpfOut);
}

//==============================================================================
// Output: Iron の実装 (Tellinen: タイトでアグレッシブなヒステリシス)
//==============================================================================
void OutputTransformer_Iron::prepare(double sampleRate) {
    fs = sampleRate; hpfState = 0.0; lastInput = 0.0; lpfState = 0.0;
    x1 = 0.0; x2 = 0.0; y1 = 0.0; y2 = 0.0;
    lastColorParam = -1.0f; lastAirParam = -1.0f; lastAgeParam = -1.0f;
    hysteresisEngine.prepare();
}

float OutputTransformer_Iron::processSample(float input, float colorParam, float airParam, float ageParam) {
    if (colorParam != lastColorParam) {
        double fcHpf = juce::jmap(static_cast<double>(colorParam), 0.0, 100.0, 20.0, 10.0);
        alphaHpf = std::exp(-juce::MathConstants<double>::twoPi * fcHpf / fs);
        hystHc = juce::jmap(static_cast<double>(colorParam), 0.0, 100.0, 0.02, 0.15);
        hystDrive = juce::jmap(static_cast<double>(colorParam), 0.0, 100.0, 1.5, 3.5);
        lastColorParam = colorParam;
    }

    if (airParam != lastAirParam) {
        double gainDb = juce::jmap(static_cast<double>(airParam), 0.0, 100.0, 0.0, 6.0);
        double A = std::pow(10.0, gainDb / 40.0);
        double w0 = juce::MathConstants<double>::twoPi * 15000.0 / fs;
        double alphaQ = std::sin(w0) / (2.0 * 0.707);
        double a0 = 1.0 + alphaQ / A;
        b0 = (1.0 + alphaQ * A) / a0; b1 = (-2.0 * std::cos(w0)) / a0; b2 = (1.0 - alphaQ * A) / a0;
        a1 = (-2.0 * std::cos(w0)) / a0; a2 = (1.0 - alphaQ / A) / a0;
        lastAirParam = airParam;
    }

    if (ageParam != lastAgeParam) {
        double fcLpf = juce::jmap(static_cast<double>(ageParam), 0.0, 100.0, 30000.0, 10000.0);
        alphaLpf = std::exp(-juce::MathConstants<double>::twoPi * fcLpf / fs);
        lastAgeParam = ageParam;
    }

    double dIn = static_cast<double>(input);

    // 変更前: double hystOut = hysteresisEngine.processSample(...)
        // 変更後↓
    double hystOut = isAnalyzerMode ? static_cast<double>(dIn)
        : hysteresisEngine.processSample(static_cast<float>(dIn), hystDrive, hystHc);

    double airOut = b0 * hystOut + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    x2 = x1; x1 = hystOut; y2 = y1; y1 = airOut;

    double lpfOut = airOut * (1.0 - alphaLpf) + alphaLpf * lpfState;
    lpfState = lpfOut;

    // ★ 出力段でのHPF (DC完全除去)
    double hpfOut = lpfOut - lastInput + alphaHpf * hpfState;
    hpfState = hpfOut; lastInput = lpfOut;

    return static_cast<float>(hpfOut);
}

//==============================================================================
// ★ Output: Nickel の実装 (J-Aモデル: 極限の透明感とクリッピング)
//==============================================================================
void OutputTransformer_Nickel::prepare(double sampleRate) {
    fs = sampleRate; hpfState = 0.0; lastInput = 0.0; lpfState = 0.0;
    x1 = 0.0; x2 = 0.0; y1 = 0.0; y2 = 0.0;
    lastColorParam = -1.0f; lastAirParam = -1.0f; lastAgeParam = -1.0f;
    hysteresisEngine.prepare();
}

float OutputTransformer_Nickel::processSample(float input, float colorParam, float airParam, float ageParam) {
    if (colorParam != lastColorParam) {
        double fcHpf = juce::jmap(static_cast<double>(colorParam), 0.0, 100.0, 5.0, 2.0);
        alphaHpf = std::exp(-juce::MathConstants<double>::twoPi * fcHpf / fs);

        // ★ J-Aモデルの再キャリブレーション (完全な透明感 -> 鋭利な飽和)
        ja_a = juce::jmap(static_cast<double>(colorParam), 0.0, 100.0, 2.0, 0.05);
        ja_k = juce::jmap(static_cast<double>(colorParam), 0.0, 100.0, 0.001, 0.03);
        ja_c = juce::jmap(static_cast<double>(colorParam), 0.0, 100.0, 0.99, 0.50);

        lastColorParam = colorParam;
    }

    if (airParam != lastAirParam) {
        double gainDb = juce::jmap(static_cast<double>(airParam), 0.0, 100.0, 0.0, 6.0);
        double A = std::pow(10.0, gainDb / 40.0);
        double w0 = juce::MathConstants<double>::twoPi * 15000.0 / fs;
        double alphaQ = std::sin(w0) / (2.0 * 0.707);
        double a0 = 1.0 + alphaQ / A;
        b0 = (1.0 + alphaQ * A) / a0; b1 = (-2.0 * std::cos(w0)) / a0; b2 = (1.0 - alphaQ * A) / a0;
        a1 = (-2.0 * std::cos(w0)) / a0; a2 = (1.0 - alphaQ / A) / a0;
        lastAirParam = airParam;
    }

    if (ageParam != lastAgeParam) {
        double fcLpf = juce::jmap(static_cast<double>(ageParam), 0.0, 100.0, 30000.0, 10000.0);
        alphaLpf = std::exp(-juce::MathConstants<double>::twoPi * fcLpf / fs);
        lastAgeParam = ageParam;
    }

    double dIn = static_cast<double>(input);

    double driveGain = juce::jmap(static_cast<double>(colorParam), 0.0, 100.0, 1.0, 2.5);

    // ★ J-Aモデルの自動メイクアップ・ゲイン (ゲイン低下問題の解決)
    // ランジュバン関数の小信号時の傾き 1/(3a) を相殺するため、入力を 3a 倍します。
    // これにより、Colorが0でも100でも常に基準音量（ユニティゲイン）が維持されます。
    double inputScaling = 3.0 * ja_a * driveGain;
    // 変更前: double hystOut = hysteresisEngine... の周辺ブロック
        // 変更後↓
    double hystOut = dIn;
    if (!isAnalyzerMode) {
        double driveGain = juce::jmap(static_cast<double>(colorParam), 0.0, 100.0, 1.0, 2.5);
        double inputScaling = 3.0 * ja_a * driveGain;
        hystOut = hysteresisEngine.processSample(static_cast<float>(dIn * inputScaling), ja_a, ja_k, ja_c);
        hystOut /= std::sqrt(driveGain);
    }

    double airOut = b0 * hystOut + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    x2 = x1; x1 = hystOut; y2 = y1; y1 = airOut;

    double lpfOut = airOut * (1.0 - alphaLpf) + alphaLpf * lpfState;
    lpfState = lpfOut;

    // 出力段でのHPF (DC完全除去)
    double hpfOut = lpfOut - lastInput + alphaHpf * hpfState;
    hpfState = hpfOut; lastInput = lpfOut;

    return static_cast<float>(hpfOut);
}