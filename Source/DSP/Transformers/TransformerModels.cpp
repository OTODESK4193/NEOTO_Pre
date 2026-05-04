#include "TransformerModels.h"
#include <cmath>
#include <algorithm>

//==============================================================================
// Input: Nickel の実装
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
// Input: Steel の実装
//==============================================================================
void InputTransformer_Steel::prepare(double sampleRate) {
    fs = sampleRate;
    lastInputADAA = 0.0;
    hpfState = 0.0; lastInput = 0.0;
    // 15Hzのハイパスフィルターで、トランス特有の低域の位相ズレを再現
    alphaHpf = std::exp(-juce::MathConstants<double>::twoPi * 15.0 / fs);
}

float InputTransformer_Steel::processSample(float input) {
    double dInput = static_cast<double>(input);

    // 1. 低域フェイズシフト (HPF)
    double hpfOut = dInput - lastInput + alphaHpf * hpfState;
    hpfState = hpfOut; lastInput = dInput;

    // 2. ADAA Softclip (Steel)
    double softclipOut = 0.0;
    double dx = hpfOut - lastInputADAA;
    if (std::abs(dx) < 1e-8) softclipOut = ADAA_Math::fx_steel((hpfOut + lastInputADAA) * 0.5);
    else softclipOut = (ADAA_Math::F1_steel(hpfOut) - ADAA_Math::F1_steel(lastInputADAA)) / dx;

    lastInputADAA = hpfOut;
    return static_cast<float>(softclipOut);
}

//==============================================================================
// Input: Iron の実装
//==============================================================================
void InputTransformer_Iron::prepare(double sampleRate) {
    fs = sampleRate;
    lastInputADAA = 0.0;
}

float InputTransformer_Iron::processSample(float input) {
    double dInput = static_cast<double>(input);
    double softclipOut = 0.0;
    double dx = dInput - lastInputADAA;

    if (std::abs(dx) < 1e-8) softclipOut = ADAA_Math::fx_iron((dInput + lastInputADAA) * 0.5);
    else softclipOut = (ADAA_Math::F1_iron(dInput) - ADAA_Math::F1_iron(lastInputADAA)) / dx;

    lastInputADAA = dInput;
    return static_cast<float>(softclipOut);
}

//==============================================================================
// Input: Amorphous の実装
//==============================================================================
void InputTransformer_Amorphous::prepare(double sampleRate) {
    fs = sampleRate;
    lastInputADAA = 0.0;
}

float InputTransformer_Amorphous::processSample(float input) {
    double dInput = static_cast<double>(input);
    double softclipOut = 0.0;
    double dx = dInput - lastInputADAA;

    // Amorphousは超広帯域・超低歪みのため、ハードクリップの閾値を高く設定してクリーンに保つ
    auto fx_amorph = [](double x) { return std::clamp(x, -4.0, 4.0); };
    auto F1_amorph = [](double x) {
        if (x > 4.0) return 4.0 * x - 8.0;
        if (x < -4.0) return -4.0 * x - 8.0;
        return 0.5 * x * x;
        };

    if (std::abs(dx) < 1e-8) softclipOut = fx_amorph((dInput + lastInputADAA) * 0.5);
    else softclipOut = (F1_amorph(dInput) - F1_amorph(lastInputADAA)) / dx;

    lastInputADAA = dInput;
    return static_cast<float>(softclipOut);
}


//==============================================================================
// Output: Steel の実装 (Neve LO1166スタイル: ワイドなヒステリシスとフェイズシフト)
//==============================================================================
void OutputTransformer_Steel::prepare(double sampleRate)
{
    fs = sampleRate;
    hpfState = 0.0; lastInput = 0.0;
    apfState = 0.0; lastApfInput = 0.0;
    lpfState = 0.0;
    x1 = 0.0; x2 = 0.0; y1 = 0.0; y2 = 0.0;

    lastColorParam = -1.0f; lastAirParam = -1.0f; lastAgeParam = -1.0f;
    hysteresisEngine.prepare();
}

float OutputTransformer_Steel::processSample(float input, float colorParam, float airParam, float ageParam)
{
    if (colorParam != lastColorParam) {
        double fcHpf = juce::jmap(static_cast<double>(colorParam), 0.0, 100.0, 15.0, 5.0);
        alphaHpf = std::exp(-juce::MathConstants<double>::twoPi * fcHpf / fs);

        double apfFreq = juce::jmap(static_cast<double>(colorParam), 0.0, 100.0, 60.0, 20.0);
        double tanVal = std::tan(juce::MathConstants<double>::pi * apfFreq / fs);
        apfAlpha = (tanVal - 1.0) / (tanVal + 1.0);

        // Colorをヒステリシスの保磁力(Hc)とドライブ(Drive)へマッピング
        hystHc = juce::jmap(static_cast<double>(colorParam), 0.0, 100.0, 0.05, 0.30);
        hystDrive = juce::jmap(static_cast<double>(colorParam), 0.0, 100.0, 1.2, 2.5);
        lastColorParam = colorParam;
    }

    if (airParam != lastAirParam) {
        double gainDb = juce::jmap(static_cast<double>(airParam), 0.0, 100.0, 0.0, 6.0);
        double A = std::pow(10.0, gainDb / 40.0);
        double w0 = juce::MathConstants<double>::twoPi * 15000.0 / fs;
        double sin_w0 = std::sin(w0);
        double cos_w0 = std::cos(w0);
        double alphaQ = sin_w0 / (2.0 * 0.707);

        double a0 = 1.0 + alphaQ / A;
        b0 = (1.0 + alphaQ * A) / a0;
        b1 = (-2.0 * cos_w0) / a0;
        b2 = (1.0 - alphaQ * A) / a0;
        a1 = (-2.0 * cos_w0) / a0;
        a2 = (1.0 - alphaQ / A) / a0;
        lastAirParam = airParam;
    }

    if (ageParam != lastAgeParam) {
        double fcLpf = juce::jmap(static_cast<double>(ageParam), 0.0, 100.0, 30000.0, 10000.0);
        alphaLpf = std::exp(-juce::MathConstants<double>::twoPi * fcLpf / fs);
        lastAgeParam = ageParam;
    }

    double dIn = static_cast<double>(input);

    // 1. 低域HPF
    double hpfOut = dIn - lastInput + alphaHpf * hpfState;
    hpfState = hpfOut; lastInput = dIn;

    // 2. 位相シフト(APF)
    double apfOut = apfAlpha * hpfOut + lastApfInput - apfAlpha * apfState;
    apfState = apfOut; lastApfInput = hpfOut;

    // 3. ヒステリシス処理 (Core Saturation)
    double hystOut = hysteresisEngine.processSample(static_cast<float>(apfOut), hystDrive, hystHc);

    // 4. Air Biquad (LC共振)
    double airOut = b0 * hystOut + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    x2 = x1; x1 = hystOut;
    y2 = y1; y1 = airOut;

    // 5. Age LPF (劣化)
    double lpfOut = airOut * (1.0 - alphaLpf) + alphaLpf * lpfState;
    lpfState = lpfOut;

    return static_cast<float>(lpfOut);
}

//==============================================================================
// Output: Iron の実装 (API 2503スタイル: タイトでアグレッシブなヒステリシス)
//==============================================================================
void OutputTransformer_Iron::prepare(double sampleRate)
{
    fs = sampleRate;
    hpfState = 0.0; lastInput = 0.0;
    lpfState = 0.0;
    x1 = 0.0; x2 = 0.0; y1 = 0.0; y2 = 0.0;
    lastColorParam = -1.0f; lastAirParam = -1.0f; lastAgeParam = -1.0f;
    hysteresisEngine.prepare();
}

float OutputTransformer_Iron::processSample(float input, float colorParam, float airParam, float ageParam)
{
    if (colorParam != lastColorParam) {
        // Ironは低域をタイトに保つためカットオフを高めに
        double fcHpf = juce::jmap(static_cast<double>(colorParam), 0.0, 100.0, 20.0, 10.0);
        alphaHpf = std::exp(-juce::MathConstants<double>::twoPi * fcHpf / fs);

        // IronはSteelよりも狭いループ(Hcが小さい)だが、サチュレーションがアグレッシブ
        hystHc = juce::jmap(static_cast<double>(colorParam), 0.0, 100.0, 0.02, 0.15);
        hystDrive = juce::jmap(static_cast<double>(colorParam), 0.0, 100.0, 1.5, 3.5);
        lastColorParam = colorParam;
    }

    if (airParam != lastAirParam) {
        double gainDb = juce::jmap(static_cast<double>(airParam), 0.0, 100.0, 0.0, 6.0);
        double A = std::pow(10.0, gainDb / 40.0);
        double w0 = juce::MathConstants<double>::twoPi * 15000.0 / fs;
        double sin_w0 = std::sin(w0);
        double cos_w0 = std::cos(w0);
        double alphaQ = sin_w0 / (2.0 * 0.707);

        double a0 = 1.0 + alphaQ / A;
        b0 = (1.0 + alphaQ * A) / a0;
        b1 = (-2.0 * cos_w0) / a0;
        b2 = (1.0 - alphaQ * A) / a0;
        a1 = (-2.0 * cos_w0) / a0;
        a2 = (1.0 - alphaQ / A) / a0;
        lastAirParam = airParam;
    }

    if (ageParam != lastAgeParam) {
        double fcLpf = juce::jmap(static_cast<double>(ageParam), 0.0, 100.0, 30000.0, 10000.0);
        alphaLpf = std::exp(-juce::MathConstants<double>::twoPi * fcLpf / fs);
        lastAgeParam = ageParam;
    }

    double dIn = static_cast<double>(input);

    // 1. 低域HPF (APFなしのタイトなボトム)
    double hpfOut = dIn - lastInput + alphaHpf * hpfState;
    hpfState = hpfOut; lastInput = dIn;

    // 2. ヒステリシス処理
    double hystOut = hysteresisEngine.processSample(static_cast<float>(hpfOut), hystDrive, hystHc);

    // 3. Air Biquad (LC共振)
    double airOut = b0 * hystOut + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    x2 = x1; x1 = hystOut;
    y2 = y1; y1 = airOut;

    // 4. Age LPF (劣化)
    double lpfOut = airOut * (1.0 - alphaLpf) + alphaLpf * lpfState;
    lpfState = lpfOut;

    return static_cast<float>(lpfOut);
}