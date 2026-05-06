#pragma execution_character_set("utf-8")
#include "PreampModels.h"
#include <cmath>
#include <algorithm>

constexpr double HEADROOM_IN = 0.125;  // 1/8にスケールダウン
constexpr double HEADROOM_OUT = 8.0;   // 8倍にスケールアップして復元
constexpr double ADAA_EPS = 1e-8;      // ADAAゼロ除算保護用閾値

//==============================================================================
// 1. API Style
//==============================================================================
void Preamp_API::prepare(double sampleRate) {
    fs = sampleRate;
    lastInputADAA1 = 0.0; lastInputADAA2 = 0.0;
    lastInputADAA_dry1 = 0.0; lastInputADAA_dry2 = 0.0;
    lastDriveParam = -1.0f; lastCharParam = -1.0f; lastAsymParam = -1.0f; lastAgeParam = -1.0f;

    alphaHpf = std::exp(-juce::MathConstants<double>::twoPi * std::min(10.0, fs * 0.49) / fs);
    alphaLpf = std::exp(-juce::MathConstants<double>::twoPi * std::min(22000.0, fs * 0.49) / fs);
    hpfState = 0.0; lpfState = 0.0; lastInput = 0.0;
    hpfState_dry = 0.0; lpfState_dry = 0.0; lastInput_dry = 0.0;
}

float Preamp_API::processDrySample(float input, float driveParam, float charParam, float asymParam, float colorParam, float ageParam) {
    double dInput = static_cast<double>(input);
    double delayed = lastInputADAA_dry1;
    lastInputADAA_dry1 = dInput;
    return static_cast<float>(delayed);
}

float Preamp_API::processSample(float input, float driveParam, float charParam, float asymParam, float colorParam, float ageParam) {
    if (driveParam != lastDriveParam) {
        double driveDb = std::pow(static_cast<double>(driveParam) / 100.0, 2.0) * 24.0;
        gain = juce::Decibels::decibelsToGain(driveDb);
        makeUp = 1.0 / (1.0 + (gain - 1.0) * 0.15);
        lastDriveParam = driveParam;
    }

    if (charParam != lastCharParam || asymParam != lastAsymParam) {
        double charNormalized = static_cast<double>(charParam) / 100.0;
        mixEven = 0.2 + (charNormalized * 0.3);
        mixOdd = 1.0 - mixEven;
        bias = juce::jmap(static_cast<double>(asymParam), 0.0, 100.0, 0.0, 0.15);
        fxBias = (ADAA_Math::fx_chebyshev_even(bias) * mixEven) + (ADAA_Math::fx_chebyshev_odd(bias) * mixOdd);
        lastCharParam = charParam; lastAsymParam = asymParam;
    }

    double drivenSignal = static_cast<double>(input) * gain * HEADROOM_IN;
    double x = drivenSignal + bias;
    double softclipOut = 0.0;

    auto calcFx = [&](double val) { return (ADAA_Math::fx_chebyshev_even(val) * mixEven) + (ADAA_Math::fx_chebyshev_odd(val) * mixOdd); };

    if (isAnalyzerMode) {
        softclipOut = calcFx(x); // ★ 修正: アナライザー表示用にも倍音を生成
        lastInputADAA2 = lastInputADAA1; lastInputADAA1 = x;
    }
    else {
        double diff1 = x - lastInputADAA1; double diff2 = lastInputADAA1 - lastInputADAA2; double diff_total = x - lastInputADAA2;
        auto calcF1 = [&](double val) { return (ADAA_Math::F1_chebyshev_even(val) * mixEven) + (ADAA_Math::F1_chebyshev_odd(val) * mixOdd); };
        auto calcF2 = [&](double val) { return (ADAA_Math::F2_chebyshev_even(val) * mixEven) + (ADAA_Math::F2_chebyshev_odd(val) * mixOdd); };

        if (std::abs(diff_total) <= ADAA_EPS) softclipOut = calcFx((x + lastInputADAA2) * 0.5);
        else if (std::abs(diff1) <= ADAA_EPS) { double div2 = (calcF2(lastInputADAA1) - calcF2(lastInputADAA2)) / diff2; softclipOut = (2.0 / diff_total) * (calcF1((x + lastInputADAA1) * 0.5) - div2); }
        else if (std::abs(diff2) <= ADAA_EPS) { double div1 = (calcF2(x) - calcF2(lastInputADAA1)) / diff1; softclipOut = (2.0 / diff_total) * (div1 - calcF1((lastInputADAA1 + lastInputADAA2) * 0.5)); }
        else { double div1 = (calcF2(x) - calcF2(lastInputADAA1)) / diff1; double div2 = (calcF2(lastInputADAA1) - calcF2(lastInputADAA2)) / diff2; softclipOut = (2.0 / diff_total) * (div1 - div2); }
        lastInputADAA2 = lastInputADAA1; lastInputADAA1 = x;
    }
    return static_cast<float>((softclipOut - fxBias) * makeUp * HEADROOM_OUT);
}

//==============================================================================
// 2. Neve Style (1073)
//==============================================================================
void Preamp_Neve::prepare(double sampleRate) {
    fs = sampleRate;
    lastInputADAA1 = 0.0; lastInputADAA2 = 0.0;
    lastInputADAA_dry1 = 0.0; lastInputADAA_dry2 = 0.0;
    lastDriveParam = -1.0f; lastCharParam = -1.0f; lastAsymParam = -1.0f; lastAgeParam = -1.0f;
}

float Preamp_Neve::processDrySample(float input, float driveParam, float charParam, float asymParam, float colorParam, float ageParam) {
    double dInput = static_cast<double>(input);
    double delayed = lastInputADAA_dry1; lastInputADAA_dry1 = dInput;
    return static_cast<float>(delayed);
}

float Preamp_Neve::processSample(float input, float driveParam, float charParam, float asymParam, float colorParam, float ageParam) {
    if (driveParam != lastDriveParam) {
        double driveDb = std::pow(static_cast<double>(driveParam) / 100.0, 3.0) * 20.0;
        gain = juce::Decibels::decibelsToGain(driveDb);
        makeUp = 1.0 / (1.0 + (gain - 1.0) * 0.1);
        lastDriveParam = driveParam;
    }
    if (charParam != lastCharParam || asymParam != lastAsymParam) {
        double norm = static_cast<double>(charParam) / 100.0;
        mixEven = 0.6 + (norm * 0.4); mixOdd = 1.0 - mixEven;
        bias = juce::jmap(static_cast<double>(asymParam), 0.0, 100.0, 0.0, 0.3);
        fxBias = (ADAA_Math::fx_chebyshev_even(bias) * mixEven) + (ADAA_Math::fx_chebyshev_odd(bias) * mixOdd);
        lastCharParam = charParam; lastAsymParam = asymParam;
    }

    double drivenSignal = static_cast<double>(input) * gain * HEADROOM_IN;
    double x = drivenSignal + bias;
    double softclipOut = 0.0;

    auto calcFx = [&](double val) { return (ADAA_Math::fx_chebyshev_even(val) * mixEven) + (ADAA_Math::fx_chebyshev_odd(val) * mixOdd); };

    if (isAnalyzerMode) {
        softclipOut = calcFx(x); // ★ 修正
        lastInputADAA2 = lastInputADAA1; lastInputADAA1 = x;
    }
    else {
        double diff1 = x - lastInputADAA1; double diff2 = lastInputADAA1 - lastInputADAA2; double diff_total = x - lastInputADAA2;
        auto calcF1 = [&](double val) { return (ADAA_Math::F1_chebyshev_even(val) * mixEven) + (ADAA_Math::F1_chebyshev_odd(val) * mixOdd); };
        auto calcF2 = [&](double val) { return (ADAA_Math::F2_chebyshev_even(val) * mixEven) + (ADAA_Math::F2_chebyshev_odd(val) * mixOdd); };

        if (std::abs(diff_total) <= ADAA_EPS) softclipOut = calcFx((x + lastInputADAA2) * 0.5);
        else if (std::abs(diff1) <= ADAA_EPS) { double div2 = (calcF2(lastInputADAA1) - calcF2(lastInputADAA2)) / diff2; softclipOut = (2.0 / diff_total) * (calcF1((x + lastInputADAA1) * 0.5) - div2); }
        else if (std::abs(diff2) <= ADAA_EPS) { double div1 = (calcF2(x) - calcF2(lastInputADAA1)) / diff1; softclipOut = (2.0 / diff_total) * (div1 - calcF1((lastInputADAA1 + lastInputADAA2) * 0.5)); }
        else { double div1 = (calcF2(x) - calcF2(lastInputADAA1)) / diff1; double div2 = (calcF2(lastInputADAA1) - calcF2(lastInputADAA2)) / diff2; softclipOut = (2.0 / diff_total) * (div1 - div2); }
        lastInputADAA2 = lastInputADAA1; lastInputADAA1 = x;
    }
    return static_cast<float>((softclipOut - fxBias) * makeUp * HEADROOM_OUT);
}

//==============================================================================
// 3. Vintage Tube Style
//==============================================================================
void Preamp_Tube::prepare(double sampleRate) {
    fs = sampleRate;
    integratorState = 0.0; lastInputADAA1 = 0.0; lastInputADAA2 = 0.0;
    lastInputADAA_dry1 = 0.0; lastInputADAA_dry2 = 0.0; lastSoftclipOut = 0.0; envState = 0.0;
    hpfState = 0.0; lastInput = 0.0; hpfState_dry = 0.0; lastInput_dry = 0.0;
    double wc = juce::MathConstants<double>::twoPi * std::min(40.0, fs * 0.49) / fs;
    apfCoef = (std::tan(wc / 2.0) - 1.0) / (std::tan(wc / 2.0) + 1.0);
    apfStateIn = 0.0; apfStateOut = 0.0; apfStateIn_dry = 0.0; apfStateOut_dry = 0.0;
    lastDriveParam = -1.0f; lastCharParam = -1.0f; lastAsymParam = -1.0f; lastAgeParam = -1.0f;
}

float Preamp_Tube::processDrySample(float input, float driveParam, float charParam, float asymParam, float colorParam, float ageParam) {
    double dInput = static_cast<double>(input);
    if (charParam != lastCharParam) {
        alphaHpf = std::exp(-juce::MathConstants<double>::twoPi * std::min(30.0, fs * 0.49) / fs);
    }
    double hpfOut = dInput - lastInput_dry + alphaHpf * hpfState_dry; hpfState_dry = hpfOut; lastInput_dry = dInput;
    double delayed = lastInputADAA_dry1; lastInputADAA_dry1 = hpfOut;
    double apfOut = apfCoef * delayed + apfStateIn_dry - apfCoef * apfStateOut_dry;
    apfStateIn_dry = delayed; apfStateOut_dry = apfOut;
    return static_cast<float>(apfOut);
}

float Preamp_Tube::processSample(float input, float driveParam, float charParam, float asymParam, float colorParam, float ageParam) {
    if (driveParam != lastDriveParam) {
        double driveDb = std::pow(static_cast<double>(driveParam) / 100.0, 3.0) * 30.0;
        gain = juce::Decibels::decibelsToGain(driveDb);
        makeUp = 1.0 / (1.0 + (gain - 1.0) * 0.1);
        lastDriveParam = driveParam;
    }
    if (charParam != lastCharParam) {
        mixEven = 0.3 + ((static_cast<double>(charParam) / 100.0) * 0.4); mixOdd = 1.0 - mixEven;
        alphaHpf = std::exp(-juce::MathConstants<double>::twoPi * std::min(30.0, fs * 0.49) / fs);
        envAttackCoef = std::exp(-1.0 / (0.02 * fs)); envReleaseCoef = std::exp(-1.0 / (0.15 * fs));
        lastCharParam = charParam;
    }
    if (asymParam != lastAsymParam) {
        bias = juce::jmap(static_cast<double>(asymParam), 0.0, 100.0, 0.0, 0.5);
        fxBias = ADAA_Math::fx_tube(bias); lastAsymParam = asymParam;
    }
    if (ageParam != lastAgeParam) {
        sagRatio = juce::jmap(static_cast<double>(ageParam), 0.0, 100.0, 0.0, 0.4);
        lastAgeParam = ageParam;
    }

    double dIn = static_cast<double>(input);
    double hpfOut = dIn - lastInput + alphaHpf * hpfState; hpfState = hpfOut; lastInput = dIn;
    double drivenSignal = hpfOut * gain;
    double absIn = std::abs(drivenSignal);
    if (absIn > envState) envState = envAttackCoef * envState + (1.0 - envAttackCoef) * absIn;
    else envState = envReleaseCoef * envState + (1.0 - envReleaseCoef) * absIn;
    drivenSignal *= std::max(0.1, 1.0 - (envState * sagRatio));
    drivenSignal *= HEADROOM_IN;

    double x = drivenSignal + bias;
    double softclipOut = 0.0;

    if (isAnalyzerMode) {
        softclipOut = ADAA_Math::fx_tube(x); // ★ 修正
        lastInputADAA2 = lastInputADAA1; lastInputADAA1 = x;
    }
    else {
        double diff1 = x - lastInputADAA1; double diff2 = lastInputADAA1 - lastInputADAA2; double diff_total = x - lastInputADAA2;
        if (std::abs(diff_total) <= ADAA_EPS) softclipOut = ADAA_Math::fx_tube((x + lastInputADAA2) * 0.5);
        else if (std::abs(diff1) <= ADAA_EPS) { double div2 = (ADAA_Math::F2_tube(lastInputADAA1) - ADAA_Math::F2_tube(lastInputADAA2)) / diff2; softclipOut = (2.0 / diff_total) * (ADAA_Math::F1_tube((x + lastInputADAA1) * 0.5) - div2); }
        else if (std::abs(diff2) <= ADAA_EPS) { double div1 = (ADAA_Math::F2_tube(x) - ADAA_Math::F2_tube(lastInputADAA1)) / diff1; softclipOut = (2.0 / diff_total) * (div1 - ADAA_Math::F1_tube((lastInputADAA1 + lastInputADAA2) * 0.5)); }
        else { double div1 = (ADAA_Math::F2_tube(x) - ADAA_Math::F2_tube(lastInputADAA1)) / diff1; double div2 = (ADAA_Math::F2_tube(lastInputADAA1) - ADAA_Math::F2_tube(lastInputADAA2)) / diff2; softclipOut = (2.0 / diff_total) * (div1 - div2); }
        lastInputADAA2 = lastInputADAA1; lastInputADAA1 = x;
    }
    double outputWithoutDC = (softclipOut - fxBias) * HEADROOM_OUT;
    double apfIn = outputWithoutDC;
    double apfOut = apfCoef * apfIn + apfStateIn - apfCoef * apfStateOut;
    apfStateIn = apfIn; apfStateOut = apfOut;
    return static_cast<float>(apfOut * makeUp);
}

//==============================================================================
// 4. SSL Modern Style
//==============================================================================
void Preamp_SSL::prepare(double sampleRate) {
    fs = sampleRate;
    lastInputADAA1 = 0.0; lastInputADAA2 = 0.0;
    lastInputADAA_dry1 = 0.0; lastInputADAA_dry2 = 0.0;
    lastDriveParam = -1.0f; lastCharParam = -1.0f; lastAsymParam = -1.0f; lastAgeParam = -1.0f;
}

float Preamp_SSL::processDrySample(float input, float driveParam, float charParam, float asymParam, float colorParam, float ageParam) {
    double dInput = static_cast<double>(input);
    double delayed = lastInputADAA_dry1; lastInputADAA_dry1 = dInput;
    return static_cast<float>(delayed);
}

float Preamp_SSL::processSample(float input, float driveParam, float charParam, float asymParam, float colorParam, float ageParam) {
    if (driveParam != lastDriveParam) {
        double driveDb = std::pow(static_cast<double>(driveParam) / 100.0, 3.0) * 15.0;
        gain = juce::Decibels::decibelsToGain(driveDb);
        makeUp = 1.0 / (1.0 + (gain - 1.0) * 0.2);
        lastDriveParam = driveParam;
    }
    if (charParam != lastCharParam || asymParam != lastAsymParam) {
        double norm = static_cast<double>(charParam) / 100.0;
        mixEven = norm * 0.3; mixOdd = 1.0 - mixEven;
        bias = juce::jmap(static_cast<double>(asymParam), 0.0, 100.0, 0.0, 0.2);
        fxBias = (ADAA_Math::fx_chebyshev_even(bias) * mixEven) + (ADAA_Math::fx_hardknee(bias) * mixOdd);
        lastCharParam = charParam; lastAsymParam = asymParam;
    }

    double drivenSignal = static_cast<double>(input) * gain * HEADROOM_IN;
    double x = drivenSignal + bias;
    double softclipOut = 0.0;

    auto calcFx = [&](double val) { return (ADAA_Math::fx_chebyshev_even(val) * mixEven) + (ADAA_Math::fx_hardknee(val) * mixOdd); };

    if (isAnalyzerMode) {
        softclipOut = calcFx(x); // ★ 修正
        lastInputADAA2 = lastInputADAA1; lastInputADAA1 = x;
    }
    else {
        double diff1 = x - lastInputADAA1; double diff2 = lastInputADAA1 - lastInputADAA2; double diff_total = x - lastInputADAA2;
        auto calcF1 = [&](double val) { return (ADAA_Math::F1_chebyshev_even(val) * mixEven) + (ADAA_Math::F1_hardknee(val) * mixOdd); };
        auto calcF2 = [&](double val) { return (ADAA_Math::F2_chebyshev_even(val) * mixEven) + (ADAA_Math::F2_hardknee(val) * mixOdd); };

        if (std::abs(diff_total) <= ADAA_EPS) softclipOut = calcFx((x + lastInputADAA2) * 0.5);
        else if (std::abs(diff1) <= ADAA_EPS) { double div2 = (calcF2(lastInputADAA1) - calcF2(lastInputADAA2)) / diff2; softclipOut = (2.0 / diff_total) * (calcF1((x + lastInputADAA1) * 0.5) - div2); }
        else if (std::abs(diff2) <= ADAA_EPS) { double div1 = (calcF2(x) - calcF2(lastInputADAA1)) / diff1; softclipOut = (2.0 / diff_total) * (div1 - calcF1((lastInputADAA1 + lastInputADAA2) * 0.5)); }
        else { double div1 = (calcF2(x) - calcF2(lastInputADAA1)) / diff1; double div2 = (calcF2(lastInputADAA1) - calcF2(lastInputADAA2)) / diff2; softclipOut = (2.0 / diff_total) * (div1 - div2); }
        lastInputADAA2 = lastInputADAA1; lastInputADAA1 = x;
    }
    return static_cast<float>((softclipOut - fxBias) * makeUp * HEADROOM_OUT);
}

//==============================================================================
// 5. TG2 Style (Modern 1) 
//==============================================================================
void Preamp_Modern1::prepare(double sampleRate) {
    fs = sampleRate;
    lastInputADAA1 = 0.0; lastInputADAA2 = 0.0; lastInputADAA_dry1 = 0.0; lastInputADAA_dry2 = 0.0;
    impLpfState = 0.0; impHpfState = 0.0; lastInput = 0.0;
    impLpfState_dry = 0.0; impHpfState_dry = 0.0; lastInput_dry = 0.0;
    color_x1 = 0.0; color_x2 = 0.0; color_y1 = 0.0; color_y2 = 0.0;
    color_x1_dry = 0.0; color_x2_dry = 0.0; color_y1_dry = 0.0; color_y2_dry = 0.0;
    lastDriveParam = -1.0f; lastCharParam = -1.0f; lastAsymParam = -1.0f; lastAgeParam = -1.0f; lastColorParam = -1.0f;
}

float Preamp_Modern1::processDrySample(float input, float driveParam, float charParam, float asymParam, float colorParam, float ageParam) {
    double dInput = static_cast<double>(input);
    double hpfOut = dInput - lastInput_dry + alphaImpedanceHpf * impHpfState_dry; impHpfState_dry = hpfOut; lastInput_dry = dInput;
    double lpfOut = hpfOut * (1.0 - alphaImpedanceLpf) + alphaImpedanceLpf * impLpfState_dry; impLpfState_dry = lpfOut;
    double delayed = lastInputADAA_dry1; lastInputADAA_dry1 = lpfOut;
    double colorOut = color_b0 * delayed + color_b1 * color_x1_dry + color_b2 * color_x2_dry - color_a1 * color_y1_dry - color_a2 * color_y2_dry;
    color_x2_dry = color_x1_dry; color_x1_dry = delayed; color_y2_dry = color_y1_dry; color_y1_dry = colorOut;
    return static_cast<float>(colorOut);
}

float Preamp_Modern1::processSample(float input, float driveParam, float charParam, float asymParam, float colorParam, float ageParam) {
    if (driveParam != lastDriveParam) {
        double driveDb = std::pow(static_cast<double>(driveParam) / 100.0, 2.0) * 28.0;
        gain = juce::Decibels::decibelsToGain(driveDb);
        makeUp = 1.0 / (1.0 + (gain - 1.0) * 0.12);
        lastDriveParam = driveParam;
    }
    if (charParam != lastCharParam || ageParam != lastAgeParam) {
        bool is300Ohm = (charParam >= 50.0f);
        double fcLpf = is300Ohm ? 16000.0 : 22000.0;
        // ★ ナイキスト・ガード追加
        double fcHpf = std::min(juce::jmap(static_cast<double>(ageParam), 0.0, 100.0, 10.0, 45.0), fs * 0.49);
        alphaImpedanceLpf = std::exp(-juce::MathConstants<double>::twoPi * std::min(fcLpf, fs * 0.49) / fs);
        alphaImpedanceHpf = std::exp(-juce::MathConstants<double>::twoPi * fcHpf / fs);
        lastCharParam = charParam; lastAgeParam = ageParam;
    }
    if (asymParam != lastAsymParam) {
        double norm = static_cast<double>(asymParam) / 100.0;
        mixEven = 0.1 + (norm * 0.4); mixOdd = 1.0 - mixEven;
        bias = norm * 0.2;
        fxBias = (ADAA_Math::fx_chebyshev_even(bias) * mixEven) + (ADAA_Math::fx_chebyshev_odd(bias) * mixOdd);
        lastAsymParam = asymParam;
    }
    if (colorParam != lastColorParam || lastColorParam == -1.0f) {
        double gainDb = juce::jmap(static_cast<double>(colorParam), 0.0, 100.0, 0.0, 2.5);
        double A = std::pow(10.0, gainDb / 40.0);
        // ★ ナイキスト・ガード追加
        double w0 = juce::MathConstants<double>::twoPi * std::min(3000.0, fs * 0.49) / fs;
        double alphaQ = std::sin(w0) / (2.0 * 0.707);
        double a0 = (A + 1.0) - (A - 1.0) * std::cos(w0) + 2.0 * std::sqrt(A) * alphaQ;
        color_b0 = A * ((A + 1.0) + (A - 1.0) * std::cos(w0) + 2.0 * std::sqrt(A) * alphaQ) / a0;
        color_b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * std::cos(w0)) / a0;
        color_b2 = A * ((A + 1.0) + (A - 1.0) * std::cos(w0) - 2.0 * std::sqrt(A) * alphaQ) / a0;
        color_a1 = 2.0 * ((A - 1.0) - (A + 1.0) * std::cos(w0)) / a0;
        color_a2 = ((A + 1.0) - (A - 1.0) * std::cos(w0) - 2.0 * std::sqrt(A) * alphaQ) / a0;
        lastColorParam = colorParam;
    }

    double dIn = static_cast<double>(input);
    double hpfOut = dIn - lastInput + alphaImpedanceHpf * impHpfState; impHpfState = hpfOut; lastInput = dIn;
    double lpfOut = hpfOut * (1.0 - alphaImpedanceLpf) + alphaImpedanceLpf * impLpfState; impLpfState = lpfOut;

    double drivenSignal = lpfOut * gain * HEADROOM_IN;
    double x = drivenSignal + bias;
    double softclipOut = 0.0;

    auto calcFx = [&](double val) { return (ADAA_Math::fx_chebyshev_even(val) * mixEven) + (ADAA_Math::fx_chebyshev_odd(val) * mixOdd); };

    if (isAnalyzerMode) {
        softclipOut = calcFx(x); // ★ 修正
        lastInputADAA2 = lastInputADAA1; lastInputADAA1 = x;
    }
    else {
        double diff1 = x - lastInputADAA1; double diff2 = lastInputADAA1 - lastInputADAA2; double diff_total = x - lastInputADAA2;
        auto calcF1 = [&](double val) { return (ADAA_Math::F1_chebyshev_even(val) * mixEven) + (ADAA_Math::F1_chebyshev_odd(val) * mixOdd); };
        auto calcF2 = [&](double val) { return (ADAA_Math::F2_chebyshev_even(val) * mixEven) + (ADAA_Math::F2_chebyshev_odd(val) * mixOdd); };

        if (std::abs(diff_total) <= ADAA_EPS) softclipOut = calcFx((x + lastInputADAA2) * 0.5);
        else if (std::abs(diff1) <= ADAA_EPS) { double div2 = (calcF2(lastInputADAA1) - calcF2(lastInputADAA2)) / diff2; softclipOut = (2.0 / diff_total) * (calcF1((x + lastInputADAA1) * 0.5) - div2); }
        else if (std::abs(diff2) <= ADAA_EPS) { double div1 = (calcF2(x) - calcF2(lastInputADAA1)) / diff1; softclipOut = (2.0 / diff_total) * (div1 - calcF1((lastInputADAA1 + lastInputADAA2) * 0.5)); }
        else { double div1 = (calcF2(x) - calcF2(lastInputADAA1)) / diff1; double div2 = (calcF2(lastInputADAA1) - calcF2(lastInputADAA2)) / diff2; softclipOut = (2.0 / diff_total) * (div1 - div2); }
        lastInputADAA2 = lastInputADAA1; lastInputADAA1 = x;
    }

    double outputWithoutDC = (softclipOut - fxBias) * makeUp * HEADROOM_OUT;
    double colorOut = color_b0 * outputWithoutDC + color_b1 * color_x1 + color_b2 * color_x2 - color_a1 * color_y1 - color_a2 * color_y2;
    color_x2 = color_x1; color_x1 = outputWithoutDC; color_y2 = color_y1; color_y1 = colorOut;
    return static_cast<float>(colorOut);
}

//==============================================================================
// 6. B173 Style (Modern 2)
//==============================================================================
void Preamp_Modern2::prepare(double sampleRate) {
    fs = sampleRate;
    lastInputADAA1 = 0.0; lastInputADAA2 = 0.0; lastInputADAA_dry1 = 0.0; lastInputADAA_dry2 = 0.0;
    lpfState = 0.0; lpfState_dry = 0.0;
    color_x1 = 0.0; color_x2 = 0.0; color_y1 = 0.0; color_y2 = 0.0;
    color_x1_dry = 0.0; color_x2_dry = 0.0; color_y1_dry = 0.0; color_y2_dry = 0.0;
    lastDriveParam = -1.0f; lastCharParam = -1.0f; lastAsymParam = -1.0f; lastAgeParam = -1.0f; lastColorParam = -1.0f;
}

float Preamp_Modern2::processDrySample(float input, float driveParam, float charParam, float asymParam, float colorParam, float ageParam) {
    double dInput = static_cast<double>(input);
    double delayed = lastInputADAA_dry1; lastInputADAA_dry1 = dInput;
    double colorOut = color_b0 * delayed + color_b1 * color_x1_dry + color_b2 * color_x2_dry - color_a1 * color_y1_dry - color_a2 * color_y2_dry;
    color_x2_dry = color_x1_dry; color_x1_dry = delayed; color_y2_dry = color_y1_dry; color_y1_dry = colorOut;
    double lpfOut = colorOut * (1.0 - alphaAgeLpf) + alphaAgeLpf * lpfState_dry; lpfState_dry = lpfOut;
    return static_cast<float>(lpfOut);
}

float Preamp_Modern2::processSample(float input, float driveParam, float charParam, float asymParam, float colorParam, float ageParam) {
    if (driveParam != lastDriveParam) {
        double driveDb = std::pow(static_cast<double>(driveParam) / 100.0, 3.0) * 22.0;
        gain = juce::Decibels::decibelsToGain(driveDb);
        makeUp = 1.0 / (1.0 + (gain - 1.0) * 0.1);
        lastDriveParam = driveParam;
    }
    if (charParam != lastCharParam || asymParam != lastAsymParam) {
        double norm = static_cast<double>(charParam) / 100.0;
        mixEven = 0.5 + (norm * 0.5);
        mixOdd = 1.0 - mixEven;
        bias = juce::jmap(static_cast<double>(asymParam), 0.0, 100.0, 0.0, 0.35);
        fxBias = (ADAA_Math::fx_chebyshev_even(bias) * mixEven) + (ADAA_Math::fx_chebyshev_odd(bias) * mixOdd);
        lastCharParam = charParam; lastAsymParam = asymParam;
    }
    if (ageParam != lastAgeParam) {
        double fcLpf = std::min(juce::jmap(static_cast<double>(ageParam), 0.0, 100.0, 22000.0, 10000.0), fs * 0.49);
        alphaAgeLpf = std::exp(-juce::MathConstants<double>::twoPi * fcLpf / fs);
        lastAgeParam = ageParam;
    }
    if (colorParam != lastColorParam || lastColorParam == -1.0f) {
        double gainDb = juce::jmap(static_cast<double>(colorParam), 0.0, 100.0, -1.5, 3.0);
        double A = std::pow(10.0, gainDb / 40.0);
        // ★ ナイキスト・ガード追加
        double w0 = juce::MathConstants<double>::twoPi * std::min(12000.0, fs * 0.49) / fs;
        double alphaQ = std::sin(w0) / (2.0 * 0.707);
        double a0 = (A + 1.0) - (A - 1.0) * std::cos(w0) + 2.0 * std::sqrt(A) * alphaQ;
        color_b0 = A * ((A + 1.0) + (A - 1.0) * std::cos(w0) + 2.0 * std::sqrt(A) * alphaQ) / a0;
        color_b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * std::cos(w0)) / a0;
        color_b2 = A * ((A + 1.0) + (A - 1.0) * std::cos(w0) - 2.0 * std::sqrt(A) * alphaQ) / a0;
        color_a1 = 2.0 * ((A - 1.0) - (A + 1.0) * std::cos(w0)) / a0;
        color_a2 = ((A + 1.0) - (A - 1.0) * std::cos(w0) - 2.0 * std::sqrt(A) * alphaQ) / a0;
        lastColorParam = colorParam;
    }

    double drivenSignal = static_cast<double>(input) * gain * HEADROOM_IN;
    double x = drivenSignal + bias;
    double softclipOut = 0.0;

    auto calcFx = [&](double val) { return (ADAA_Math::fx_chebyshev_even(val) * mixEven) + (ADAA_Math::fx_chebyshev_odd(val) * mixOdd); };

    if (isAnalyzerMode) {
        softclipOut = calcFx(x); // ★ 修正
        lastInputADAA2 = lastInputADAA1; lastInputADAA1 = x;
    }
    else {
        double diff1 = x - lastInputADAA1; double diff2 = lastInputADAA1 - lastInputADAA2; double diff_total = x - lastInputADAA2;
        auto calcF1 = [&](double val) { return (ADAA_Math::F1_chebyshev_even(val) * mixEven) + (ADAA_Math::F1_chebyshev_odd(val) * mixOdd); };
        auto calcF2 = [&](double val) { return (ADAA_Math::F2_chebyshev_even(val) * mixEven) + (ADAA_Math::F2_chebyshev_odd(val) * mixOdd); };

        if (std::abs(diff_total) <= ADAA_EPS) softclipOut = calcFx((x + lastInputADAA2) * 0.5);
        else if (std::abs(diff1) <= ADAA_EPS) { double div2 = (calcF2(lastInputADAA1) - calcF2(lastInputADAA2)) / diff2; softclipOut = (2.0 / diff_total) * (calcF1((x + lastInputADAA1) * 0.5) - div2); }
        else if (std::abs(diff2) <= ADAA_EPS) { double div1 = (calcF2(x) - calcF2(lastInputADAA1)) / diff1; softclipOut = (2.0 / diff_total) * (div1 - calcF1((lastInputADAA1 + lastInputADAA2) * 0.5)); }
        else { double div1 = (calcF2(x) - calcF2(lastInputADAA1)) / diff1; double div2 = (calcF2(lastInputADAA1) - calcF2(lastInputADAA2)) / diff2; softclipOut = (2.0 / diff_total) * (div1 - div2); }
        lastInputADAA2 = lastInputADAA1; lastInputADAA1 = x;
    }

    double outputWithoutDC = (softclipOut - fxBias) * makeUp * HEADROOM_OUT;
    double colorOut = color_b0 * outputWithoutDC + color_b1 * color_x1 + color_b2 * color_x2 - color_a1 * color_y1 - color_a2 * color_y2;
    color_x2 = color_x1; color_x1 = outputWithoutDC; color_y2 = color_y1; color_y1 = colorOut;

    double lpfOut = colorOut * (1.0 - alphaAgeLpf) + alphaAgeLpf * lpfState; lpfState = lpfOut;
    return static_cast<float>(lpfOut);
}