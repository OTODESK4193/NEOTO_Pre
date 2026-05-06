#pragma execution_character_set("utf-8")
#include "TransformerModels.h"
#include <cmath>
#include <algorithm>

//==============================================================================
// Input: Nickel
//==============================================================================
void InputTransformer_Nickel::prepare(double sampleRate) { fs = sampleRate; lastInputADAA = 0.0; lastInputADAA_dry = 0.0; }
float InputTransformer_Nickel::processSample(float input) {
    double dIn = static_cast<double>(input); double dx = dIn - lastInputADAA; double out = 0.0;
    if (std::abs(dx) < 1e-8) out = ADAA_Math::fx_hardclip((dIn + lastInputADAA) * 0.5);
    else out = (ADAA_Math::F1_hardclip(dIn) - ADAA_Math::F1_hardclip(lastInputADAA)) / dx;
    lastInputADAA = dIn; return static_cast<float>(out);
}
float InputTransformer_Nickel::processDrySample(float input) {
    double dIn = static_cast<double>(input);
    double out = (dIn + lastInputADAA_dry) * 0.5;
    lastInputADAA_dry = dIn; return static_cast<float>(out);
}

//==============================================================================
// Input: Steel
//==============================================================================
void InputTransformer_Steel::prepare(double sampleRate) {
    fs = sampleRate; lastInputADAA = 0.0; hpfState = 0.0; lastInput = 0.0;
    lastInputADAA_dry = 0.0; hpfState_dry = 0.0; lastInput_dry = 0.0;
    alphaHpf = std::exp(-juce::MathConstants<double>::twoPi * std::min(15.0, fs * 0.49) / fs);
}
float InputTransformer_Steel::processSample(float input) {
    double dIn = static_cast<double>(input);
    double hpfOut = dIn - lastInput + alphaHpf * hpfState; hpfState = hpfOut; lastInput = dIn;
    double out = 0.0; double dx = hpfOut - lastInputADAA;
    if (std::abs(dx) < 1e-8) out = ADAA_Math::fx_steel((hpfOut + lastInputADAA) * 0.5);
    else out = (ADAA_Math::F1_steel(hpfOut) - ADAA_Math::F1_steel(lastInputADAA)) / dx;
    lastInputADAA = hpfOut; return static_cast<float>(out);
}
float InputTransformer_Steel::processDrySample(float input) {
    double dIn = static_cast<double>(input);
    double hpfOut = dIn - lastInput_dry + alphaHpf * hpfState_dry; hpfState_dry = hpfOut; lastInput_dry = dIn;
    double out = (hpfOut + lastInputADAA_dry) * 0.5;
    lastInputADAA_dry = hpfOut; return static_cast<float>(out);
}

//==============================================================================
// Input: Iron
//==============================================================================
void InputTransformer_Iron::prepare(double sampleRate) { fs = sampleRate; lastInputADAA = 0.0; lastInputADAA_dry = 0.0; }
float InputTransformer_Iron::processSample(float input) {
    double dIn = static_cast<double>(input); double dx = dIn - lastInputADAA; double out = 0.0;
    if (std::abs(dx) < 1e-8) out = ADAA_Math::fx_iron((dIn + lastInputADAA) * 0.5);
    else out = (ADAA_Math::F1_iron(dIn) - ADAA_Math::F1_iron(lastInputADAA)) / dx;
    lastInputADAA = dIn; return static_cast<float>(out);
}
float InputTransformer_Iron::processDrySample(float input) {
    double dIn = static_cast<double>(input);
    double out = (dIn + lastInputADAA_dry) * 0.5;
    lastInputADAA_dry = dIn; return static_cast<float>(out);
}

//==============================================================================
// Input: Amorphous
//==============================================================================
void InputTransformer_Amorphous::prepare(double sampleRate) { fs = sampleRate; lastInputADAA = 0.0; lastInputADAA_dry = 0.0; }
float InputTransformer_Amorphous::processSample(float input) {
    double dIn = static_cast<double>(input); double dx = dIn - lastInputADAA; double out = 0.0;
    auto fx_a = [](double x) { return std::clamp(x, -4.0, 4.0); };
    auto F1_a = [](double x) { if (x > 4.0) return 4.0 * x - 8.0; if (x < -4.0) return -4.0 * x - 8.0; return 0.5 * x * x; };
    if (std::abs(dx) < 1e-8) out = fx_a((dIn + lastInputADAA) * 0.5);
    else out = (F1_a(dIn) - F1_a(lastInputADAA)) / dx;
    lastInputADAA = dIn; return static_cast<float>(out);
}
float InputTransformer_Amorphous::processDrySample(float input) {
    double dIn = static_cast<double>(input);
    double out = (dIn + lastInputADAA_dry) * 0.5;
    lastInputADAA_dry = dIn; return static_cast<float>(out);
}

//==============================================================================
// Output: Amorphous
//==============================================================================
void OutputTransformer_Amorphous::prepare(double sampleRate) {
    fs = sampleRate;
    x1 = 0.0; x2 = 0.0; y1 = 0.0; y2 = 0.0; lpfState = 0.0;
    x1_dry = 0.0; x2_dry = 0.0; y1_dry = 0.0; y2_dry = 0.0; lpfState_dry = 0.0;
    lastColorParam = -1.0f; lastAirParam = -1.0f; lastAgeParam = -1.0f;
}
float OutputTransformer_Amorphous::processSample(float input, float colorParam, float airParam, float ageParam) {
    if (colorParam != lastColorParam) {
        double cv = std::pow(static_cast<double>(colorParam) / 100.0, 3.0);
        threshold = juce::jmap(cv, 0.0, 1.0, 5.0, 0.35); driveGain = juce::jmap(cv, 0.0, 1.0, 1.0, 3.5);
        lastColorParam = colorParam;
    }
    if (airParam != lastAirParam) {
        double gainDb = juce::jmap(static_cast<double>(airParam), 0.0, 100.0, 0.0, 6.0);
        double A = std::pow(10.0, gainDb / 40.0);
        double w0 = juce::MathConstants<double>::twoPi * std::min(20000.0, fs * 0.49) / fs; // ★ ナイキスト保護
        double alphaQ = std::sin(w0) / (2.0 * 0.707); double a0 = 1.0 + alphaQ / A;
        b0 = (1.0 + alphaQ * A) / a0; b1 = (-2.0 * std::cos(w0)) / a0; b2 = (1.0 - alphaQ * A) / a0;
        a1 = (-2.0 * std::cos(w0)) / a0; a2 = (1.0 - alphaQ / A) / a0; lastAirParam = airParam;
    }
    if (ageParam != lastAgeParam) {
        double fcLpf = std::min(juce::jmap(static_cast<double>(ageParam), 0.0, 100.0, 35000.0, 12000.0), fs * 0.49);
        alphaLpf = std::exp(-juce::MathConstants<double>::twoPi * fcLpf / fs); lastAgeParam = ageParam;
    }

    double out = static_cast<double>(input);
    // アナライザー表示にもクリッピング特性を反映させる
    out *= driveGain; out = std::clamp(out, -threshold, threshold); out /= driveGain;

    double airOut = b0 * out + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    x2 = x1; x1 = out; y2 = y1; y1 = airOut;
    double lpfOut = airOut * (1.0 - alphaLpf) + alphaLpf * lpfState; lpfState = lpfOut;
    return static_cast<float>(lpfOut);
}
float OutputTransformer_Amorphous::processDrySample(float input, float airParam, float ageParam) {
    double out = static_cast<double>(input);
    double airOut = b0 * out + b1 * x1_dry + b2 * x2_dry - a1 * y1_dry - a2 * y2_dry;
    x2_dry = x1_dry; x1_dry = out; y2_dry = y1_dry; y1_dry = airOut;
    double lpfOut = airOut * (1.0 - alphaLpf) + alphaLpf * lpfState_dry; lpfState_dry = lpfOut;
    return static_cast<float>(lpfOut);
}

//==============================================================================
// Output: Steel
//==============================================================================
void OutputTransformer_Steel::prepare(double sampleRate) {
    fs = sampleRate;
    hpfState = 0.0; lastInput = 0.0; apfState = 0.0; lastApfInput = 0.0; lpfState = 0.0;
    x1 = 0.0; x2 = 0.0; y1 = 0.0; y2 = 0.0;
    hpfState_dry = 0.0; lastInput_dry = 0.0; apfState_dry = 0.0; lastApfInput_dry = 0.0; lpfState_dry = 0.0;
    x1_dry = 0.0; x2_dry = 0.0; y1_dry = 0.0; y2_dry = 0.0;
    lastColorParam = -1.0f; lastAirParam = -1.0f; lastAgeParam = -1.0f;
    hysteresisEngine.prepare();
}
float OutputTransformer_Steel::processSample(float input, float colorParam, float airParam, float ageParam) {
    if (colorParam != lastColorParam) {
        double cv = std::pow(static_cast<double>(colorParam) / 100.0, 3.0);
        double fcHpf = std::min(juce::jmap(cv, 0.0, 1.0, 15.0, 5.0), fs * 0.49);
        alphaHpf = std::exp(-juce::MathConstants<double>::twoPi * fcHpf / fs);
        double apfFreq = std::min(juce::jmap(cv, 0.0, 1.0, 60.0, 20.0), fs * 0.49);
        double tanVal = std::tan(juce::MathConstants<double>::pi * apfFreq / fs);
        apfAlpha = (tanVal - 1.0) / (tanVal + 1.0);
        hystHc = juce::jmap(cv, 0.0, 1.0, 0.001, 0.30); hystDrive = juce::jmap(cv, 0.0, 1.0, 1.0, 2.5);
        lastColorParam = colorParam;
    }
    if (airParam != lastAirParam) {
        double gainDb = juce::jmap(static_cast<double>(airParam), 0.0, 100.0, 0.0, 6.0);
        double A = std::pow(10.0, gainDb / 40.0);
        double w0 = juce::MathConstants<double>::twoPi * std::min(15000.0, fs * 0.49) / fs; // ★ ナイキスト保護
        double alphaQ = std::sin(w0) / (2.0 * 0.707); double a0 = 1.0 + alphaQ / A;
        b0 = (1.0 + alphaQ * A) / a0; b1 = (-2.0 * std::cos(w0)) / a0; b2 = (1.0 - alphaQ * A) / a0;
        a1 = (-2.0 * std::cos(w0)) / a0; a2 = (1.0 - alphaQ / A) / a0; lastAirParam = airParam;
    }
    if (ageParam != lastAgeParam) {
        double fcLpf = std::min(juce::jmap(static_cast<double>(ageParam), 0.0, 100.0, 30000.0, 10000.0), fs * 0.49);
        alphaLpf = std::exp(-juce::MathConstants<double>::twoPi * fcLpf / fs); lastAgeParam = ageParam;
    }

    double dIn = static_cast<double>(input);
    double apfOut = apfAlpha * dIn + lastApfInput - apfAlpha * apfState; apfState = apfOut; lastApfInput = dIn;
    // ★ 修正: アナライザーモードでもヒステリシスを通し倍音を表示させる
    double hystOut = hysteresisEngine.processSample(static_cast<float>(apfOut), hystDrive, hystHc);

    double airOut = b0 * hystOut + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    x2 = x1; x1 = hystOut; y2 = y1; y1 = airOut;
    double lpfOut = airOut * (1.0 - alphaLpf) + alphaLpf * lpfState; lpfState = lpfOut;
    double hpfOut = lpfOut - lastInput + alphaHpf * hpfState; hpfState = hpfOut; lastInput = lpfOut;
    return static_cast<float>(hpfOut);
}
float OutputTransformer_Steel::processDrySample(float input, float airParam, float ageParam) {
    double dIn = static_cast<double>(input);
    double apfOut = apfAlpha * dIn + lastApfInput_dry - apfAlpha * apfState_dry; apfState_dry = apfOut; lastApfInput_dry = dIn;
    double airOut = b0 * apfOut + b1 * x1_dry + b2 * x2_dry - a1 * y1_dry - a2 * y2_dry;
    x2_dry = x1_dry; x1_dry = apfOut; y2_dry = y1_dry; y1_dry = airOut;
    double lpfOut = airOut * (1.0 - alphaLpf) + alphaLpf * lpfState_dry; lpfState_dry = lpfOut;
    double hpfOut = lpfOut - lastInput_dry + alphaHpf * hpfState_dry; hpfState_dry = hpfOut; lastInput_dry = lpfOut;
    return static_cast<float>(hpfOut);
}

//==============================================================================
// Output: Iron
//==============================================================================
void OutputTransformer_Iron::prepare(double sampleRate) {
    fs = sampleRate;
    hpfState = 0.0; lastInput = 0.0; lpfState = 0.0; x1 = 0.0; x2 = 0.0; y1 = 0.0; y2 = 0.0;
    hpfState_dry = 0.0; lastInput_dry = 0.0; lpfState_dry = 0.0; x1_dry = 0.0; x2_dry = 0.0; y1_dry = 0.0; y2_dry = 0.0;
    lastColorParam = -1.0f; lastAirParam = -1.0f; lastAgeParam = -1.0f; hysteresisEngine.prepare();
}
float OutputTransformer_Iron::processSample(float input, float colorParam, float airParam, float ageParam) {
    if (colorParam != lastColorParam) {
        double cv = std::pow(static_cast<double>(colorParam) / 100.0, 3.0);
        double fcHpf = std::min(juce::jmap(cv, 0.0, 1.0, 20.0, 10.0), fs * 0.49);
        alphaHpf = std::exp(-juce::MathConstants<double>::twoPi * fcHpf / fs);
        hystHc = juce::jmap(cv, 0.0, 1.0, 0.001, 0.30); hystDrive = juce::jmap(cv, 0.0, 1.0, 1.0, 2.5);
        lastColorParam = colorParam;
    }
    if (airParam != lastAirParam) {
        double gainDb = juce::jmap(static_cast<double>(airParam), 0.0, 100.0, 0.0, 6.0);
        double A = std::pow(10.0, gainDb / 40.0);
        double w0 = juce::MathConstants<double>::twoPi * std::min(15000.0, fs * 0.49) / fs;
        double alphaQ = std::sin(w0) / (2.0 * 0.707); double a0 = 1.0 + alphaQ / A;
        b0 = (1.0 + alphaQ * A) / a0; b1 = (-2.0 * std::cos(w0)) / a0; b2 = (1.0 - alphaQ * A) / a0;
        a1 = (-2.0 * std::cos(w0)) / a0; a2 = (1.0 - alphaQ / A) / a0; lastAirParam = airParam;
    }
    if (ageParam != lastAgeParam) {
        double fcLpf = std::min(juce::jmap(static_cast<double>(ageParam), 0.0, 100.0, 30000.0, 10000.0), fs * 0.49);
        alphaLpf = std::exp(-juce::MathConstants<double>::twoPi * fcLpf / fs); lastAgeParam = ageParam;
    }
    double dIn = static_cast<double>(input);
    double hystOut = hysteresisEngine.processSample(static_cast<float>(dIn), hystDrive, hystHc);

    double airOut = b0 * hystOut + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    x2 = x1; x1 = hystOut; y2 = y1; y1 = airOut;
    double lpfOut = airOut * (1.0 - alphaLpf) + alphaLpf * lpfState; lpfState = lpfOut;
    double hpfOut = lpfOut - lastInput + alphaHpf * hpfState; hpfState = hpfOut; lastInput = lpfOut;
    return static_cast<float>(hpfOut);
}
float OutputTransformer_Iron::processDrySample(float input, float airParam, float ageParam) {
    double dIn = static_cast<double>(input);
    double airOut = b0 * dIn + b1 * x1_dry + b2 * x2_dry - a1 * y1_dry - a2 * y2_dry;
    x2_dry = x1_dry; x1_dry = dIn; y2_dry = y1_dry; y1_dry = airOut;
    double lpfOut = airOut * (1.0 - alphaLpf) + alphaLpf * lpfState_dry; lpfState_dry = lpfOut;
    double hpfOut = lpfOut - lastInput_dry + alphaHpf * hpfState_dry; hpfState_dry = hpfOut; lastInput_dry = lpfOut;
    return static_cast<float>(hpfOut);
}

//==============================================================================
// Output: Nickel
//==============================================================================
void OutputTransformer_Nickel::prepare(double sampleRate) {
    fs = sampleRate;
    hpfState = 0.0; lastInput = 0.0; lpfState = 0.0; x1 = 0.0; x2 = 0.0; y1 = 0.0; y2 = 0.0;
    hpfState_dry = 0.0; lastInput_dry = 0.0; lpfState_dry = 0.0; x1_dry = 0.0; x2_dry = 0.0; y1_dry = 0.0; y2_dry = 0.0;
    lastColorParam = -1.0f; lastAirParam = -1.0f; lastAgeParam = -1.0f; hysteresisEngine.prepare();
}
float OutputTransformer_Nickel::processSample(float input, float colorParam, float airParam, float ageParam) {
    if (colorParam != lastColorParam) {
        double cv = std::pow(static_cast<double>(colorParam) / 100.0, 3.0);
        double fcHpf = std::min(juce::jmap(cv, 0.0, 1.0, 5.0, 2.0), fs * 0.49);
        alphaHpf = std::exp(-juce::MathConstants<double>::twoPi * fcHpf / fs);
        ja_a = juce::jmap(cv, 0.0, 1.0, 30.0, 0.05); ja_k = juce::jmap(cv, 0.0, 1.0, 0.00001, 0.04); ja_c = juce::jmap(cv, 0.0, 1.0, 0.9999, 0.45);
        lastColorParam = colorParam;
    }
    if (airParam != lastAirParam) {
        double gainDb = juce::jmap(static_cast<double>(airParam), 0.0, 100.0, 0.0, 6.0);
        double A = std::pow(10.0, gainDb / 40.0);
        double w0 = juce::MathConstants<double>::twoPi * std::min(15000.0, fs * 0.49) / fs;
        double alphaQ = std::sin(w0) / (2.0 * 0.707); double a0 = 1.0 + alphaQ / A;
        b0 = (1.0 + alphaQ * A) / a0; b1 = (-2.0 * std::cos(w0)) / a0; b2 = (1.0 - alphaQ * A) / a0;
        a1 = (-2.0 * std::cos(w0)) / a0; a2 = (1.0 - alphaQ / A) / a0; lastAirParam = airParam;
    }
    if (ageParam != lastAgeParam) {
        double fcLpf = std::min(juce::jmap(static_cast<double>(ageParam), 0.0, 100.0, 35000.0, 10000.0), fs * 0.49);
        alphaLpf = std::exp(-juce::MathConstants<double>::twoPi * fcLpf / fs); lastAgeParam = ageParam;
    }
    double dIn = static_cast<double>(input); double hystOut = dIn;

    double cv = std::pow(static_cast<double>(colorParam) / 100.0, 3.0);
    double df = juce::jmap(cv, 0.0, 1.0, 1.0, 2.5);
    double sc = 3.0 * ja_a;
    hystOut = hysteresisEngine.processSample(static_cast<float>(dIn * sc * df), ja_a, ja_k, ja_c);
    hystOut /= std::sqrt(df);

    double airOut = b0 * hystOut + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    x2 = x1; x1 = hystOut; y2 = y1; y1 = airOut;
    double lpfOut = airOut * (1.0 - alphaLpf) + alphaLpf * lpfState; lpfState = lpfOut;
    double hpfOut = lpfOut - lastInput + alphaHpf * hpfState; hpfState = hpfOut; lastInput = lpfOut;
    return static_cast<float>(hpfOut);
}
float OutputTransformer_Nickel::processDrySample(float input, float airParam, float ageParam) {
    double dIn = static_cast<double>(input);
    double airOut = b0 * dIn + b1 * x1_dry + b2 * x2_dry - a1 * y1_dry - a2 * y2_dry;
    x2_dry = x1_dry; x1_dry = dIn; y2_dry = y1_dry; y1_dry = airOut;
    double lpfOut = airOut * (1.0 - alphaLpf) + alphaLpf * lpfState_dry; lpfState_dry = lpfOut;
    double hpfOut = lpfOut - lastInput_dry + alphaHpf * hpfState_dry; hpfState_dry = hpfOut; lastInput_dry = lpfOut;
    return static_cast<float>(hpfOut);
}

// Carnhill: Iron に類似した低域依存の歪み
void InputTransformer_Carnhill::prepare(double sampleRate) { fs = sampleRate; lastInputADAA = 0.0; lastInputADAA_dry = 0.0; }
float InputTransformer_Carnhill::processSample(float input) {
    double dIn = static_cast<double>(input); double dx = dIn - lastInputADAA; double out = 0.0;
    if (std::abs(dx) < 1e-8) out = ADAA_Math::fx_iron((dIn + lastInputADAA) * 0.5);
    else out = (ADAA_Math::F1_iron(dIn) - ADAA_Math::F1_iron(lastInputADAA)) / dx;
    lastInputADAA = dIn; return static_cast<float>(out);
}
float InputTransformer_Carnhill::processDrySample(float input) {
    double dIn = static_cast<double>(input); double out = (dIn + lastInputADAA_dry) * 0.5;
    lastInputADAA_dry = dIn; return static_cast<float>(out);
}

// Cinemag: 非常にクリアで広ヘッドルーム
void InputTransformer_Cinemag::prepare(double sampleRate) { fs = sampleRate; lastInputADAA = 0.0; lastInputADAA_dry = 0.0; }
float InputTransformer_Cinemag::processSample(float input) {
    double dIn = static_cast<double>(input); double dx = dIn - lastInputADAA; double out = 0.0;
    if (std::abs(dx) < 1e-8) out = ADAA_Math::fx_hardclip((dIn + lastInputADAA) * 0.5);
    else out = (ADAA_Math::F1_hardclip(dIn) - ADAA_Math::F1_hardclip(lastInputADAA)) / dx;
    lastInputADAA = dIn; return static_cast<float>(out);
}
float InputTransformer_Cinemag::processDrySample(float input) {
    double dIn = static_cast<double>(input); double out = (dIn + lastInputADAA_dry) * 0.5;
    lastInputADAA_dry = dIn; return static_cast<float>(out);
}

//==============================================================================
// Output: Carnhill (TG2 Style)
//==============================================================================
void OutputTransformer_Carnhill::prepare(double sampleRate) {
    fs = sampleRate;
    lastColorParam = -1.0f; lastAirParam = -1.0f; lastAgeParam = -1.0f;
    hpfState = 0.0; lpfState = 0.0; lastInput = 0.0;
    hpfState_dry = 0.0; lpfState_dry = 0.0; lastInput_dry = 0.0;

    col_x1 = 0.0; col_x2 = 0.0; col_y1 = 0.0; col_y2 = 0.0;
    col_x1_dry = 0.0; col_x2_dry = 0.0; col_y1_dry = 0.0; col_y2_dry = 0.0;

    air_x1 = 0.0; air_x2 = 0.0; air_y1 = 0.0; air_y2 = 0.0;
    air_x1_dry = 0.0; air_x2_dry = 0.0; air_y1_dry = 0.0; air_y2_dry = 0.0;

    hysteresisEngine.prepare();
}

float OutputTransformer_Carnhill::processSample(float input, float colorParam, float airParam, float ageParam) {
    // Color: 3.5kHz High Pump (最大2.5dB)
    if (colorParam != lastColorParam) {
        double cv = static_cast<double>(colorParam) / 100.0;
        double gainDb = cv * 2.5;
        double A = std::pow(10.0, gainDb / 40.0);
        double w0 = juce::MathConstants<double>::twoPi * std::min(3500.0, fs * 0.49) / fs;
        double alpha = std::sin(w0) / 2.0 * 0.707;
        double a0 = (A + 1.0) - (A - 1.0) * std::cos(w0) + 2.0 * std::sqrt(A) * alpha;
        col_b0 = (A * ((A + 1.0) + (A - 1.0) * std::cos(w0) + 2.0 * std::sqrt(A) * alpha)) / a0;
        col_b1 = (-2.0 * A * ((A - 1.0) + (A + 1.0) * std::cos(w0))) / a0;
        col_b2 = (A * ((A + 1.0) + (A - 1.0) * std::cos(w0) - 2.0 * std::sqrt(A) * alpha)) / a0;
        col_a1 = (2.0 * ((A - 1.0) - (A + 1.0) * std::cos(w0))) / a0;
        col_a2 = ((A + 1.0) - (A - 1.0) * std::cos(w0) - 2.0 * std::sqrt(A) * alpha) / a0;

        ja_a = juce::jmap(cv, 0.0, 1.0, 35.0, 10.0);
        ja_k = juce::jmap(cv, 0.0, 1.0, 15.0, 50.0);
        lastColorParam = colorParam;
    }
    // Air: 25kHz Resonance Peak
    if (airParam != lastAirParam) {
        double cv = static_cast<double>(airParam) / 100.0;
        double gainDb = cv * 3.0;
        double A = std::pow(10.0, gainDb / 40.0);
        double w0 = juce::MathConstants<double>::twoPi * std::min(25000.0, fs * 0.49) / fs;
        double alpha = std::sin(w0) / (2.0 * 0.5); // Q=0.5
        double a0 = 1.0 + alpha / A;
        air_b0 = (1.0 + alpha * A) / a0;
        air_b1 = (-2.0 * std::cos(w0)) / a0;
        air_b2 = (1.0 - alpha * A) / a0;
        air_a1 = (-2.0 * std::cos(w0)) / a0;
        air_a2 = (1.0 - alpha / A) / a0;
        lastAirParam = airParam;
    }
    // Age: Low-end Roll-off & Magnetic Lag LPF
    if (ageParam != lastAgeParam) {
        double cv = static_cast<double>(ageParam) / 100.0;
        double fcHpf = std::min(juce::jmap(cv, 0.0, 1.0, 10.0, 45.0), fs * 0.49);
        alphaHpf = std::exp(-juce::MathConstants<double>::twoPi * fcHpf / fs);
        double fcLpf = std::min(juce::jmap(cv, 0.0, 1.0, 32000.0, 12000.0), fs * 0.49);
        alphaLpf = std::exp(-juce::MathConstants<double>::twoPi * fcLpf / fs);
        lastAgeParam = ageParam;
    }

    double dIn = static_cast<double>(input);

    // Hysteresis (Analyzer bypass)
    double hystOut = isAnalyzerMode ? dIn : hysteresisEngine.processSample(static_cast<float>(dIn * 2.0), ja_a, ja_k, ja_c) * 0.5;

    // Color (High Shelf)
    double colOut = col_b0 * hystOut + col_b1 * col_x1 + col_b2 * col_x2 - col_a1 * col_y1 - col_a2 * col_y2;
    col_x2 = col_x1; col_x1 = hystOut; col_y2 = col_y1; col_y1 = colOut;

    // Air (Peak)
    double airOut = air_b0 * colOut + air_b1 * air_x1 + air_b2 * air_x2 - air_a1 * air_y1 - air_a2 * air_y2;
    air_x2 = air_x1; air_x1 = colOut; air_y2 = air_y1; air_y1 = airOut;

    // Age (LPF & HPF)
    double lpfOut = airOut * (1.0 - alphaLpf) + alphaLpf * lpfState; lpfState = lpfOut;
    double hpfOut = lpfOut - lastInput + alphaHpf * hpfState; hpfState = hpfOut; lastInput = lpfOut;

    return static_cast<float>(hpfOut);
}

float OutputTransformer_Carnhill::processDrySample(float input, float airParam, float ageParam) {
    double dIn = static_cast<double>(input);

    // Color Dry
    double colOut = col_b0 * dIn + col_b1 * col_x1_dry + col_b2 * col_x2_dry - col_a1 * col_y1_dry - col_a2 * col_y2_dry;
    col_x2_dry = col_x1_dry; col_x1_dry = dIn; col_y2_dry = col_y1_dry; col_y1_dry = colOut;

    // Air Dry
    double airOut = air_b0 * colOut + air_b1 * air_x1_dry + air_b2 * air_x2_dry - air_a1 * air_y1_dry - air_a2 * air_y2_dry;
    air_x2_dry = air_x1_dry; air_x1_dry = colOut; air_y2_dry = air_y1_dry; air_y1_dry = airOut;

    // Age Dry
    double lpfOut = airOut * (1.0 - alphaLpf) + alphaLpf * lpfState_dry; lpfState_dry = lpfOut;
    double hpfOut = lpfOut - lastInput_dry + alphaHpf * hpfState_dry; hpfState_dry = hpfOut; lastInput_dry = lpfOut;

    return static_cast<float>(hpfOut);
}

//==============================================================================
// Output: Cinemag (B173 Style)
//==============================================================================
void OutputTransformer_Cinemag::prepare(double sampleRate) {
    fs = sampleRate;
    lastColorParam = -1.0f; lastAirParam = -1.0f; lastAgeParam = -1.0f;
    hpfState = 0.0; lpfState = 0.0; lastInput = 0.0;
    hpfState_dry = 0.0; lpfState_dry = 0.0; lastInput_dry = 0.0;

    colH_x1 = 0.0; colH_x2 = 0.0; colH_y1 = 0.0; colH_y2 = 0.0;
    colH_x1_dry = 0.0; colH_x2_dry = 0.0; colH_y1_dry = 0.0; colH_y2_dry = 0.0;

    colL_x1 = 0.0; colL_x2 = 0.0; colL_y1 = 0.0; colL_y2 = 0.0;
    colL_x1_dry = 0.0; colL_x2_dry = 0.0; colL_y1_dry = 0.0; colL_y2_dry = 0.0;

    air_x1 = 0.0; air_x2 = 0.0; air_y1 = 0.0; air_y2 = 0.0;
    air_x1_dry = 0.0; air_x2_dry = 0.0; air_y1_dry = 0.0; air_y2_dry = 0.0;

    hysteresisEngine.prepare();
}

float OutputTransformer_Cinemag::processSample(float input, float colorParam, float airParam, float ageParam) {
    if (colorParam != lastColorParam) {
        double cv = static_cast<double>(colorParam) / 100.0;
        // Low Shelf (80Hz)
        double gainDbL = cv * 1.5;
        double AL = std::pow(10.0, gainDbL / 40.0);
        double w0L = juce::MathConstants<double>::twoPi * std::min(80.0, fs * 0.49) / fs;
        double alphaL = std::sin(w0L) / 2.0 * 0.707;
        double a0L = (AL + 1.0) + (AL - 1.0) * std::cos(w0L) + 2.0 * std::sqrt(AL) * alphaL;
        colL_b0 = (AL * ((AL + 1.0) - (AL - 1.0) * std::cos(w0L) + 2.0 * std::sqrt(AL) * alphaL)) / a0L;
        colL_b1 = (2.0 * AL * ((AL - 1.0) - (AL + 1.0) * std::cos(w0L))) / a0L;
        colL_b2 = (AL * ((AL + 1.0) - (AL - 1.0) * std::cos(w0L) - 2.0 * std::sqrt(AL) * alphaL)) / a0L;
        colL_a1 = (-2.0 * ((AL - 1.0) + (AL + 1.0) * std::cos(w0L))) / a0L;
        colL_a2 = ((AL + 1.0) + (AL - 1.0) * std::cos(w0L) - 2.0 * std::sqrt(AL) * alphaL) / a0L;

        // High Shelf (12kHz)
        double gainDbH = cv * 1.5;
        double AH = std::pow(10.0, gainDbH / 40.0);
        double w0H = juce::MathConstants<double>::twoPi * std::min(12000.0, fs * 0.49) / fs;
        double alphaH = std::sin(w0H) / 2.0 * 0.707;
        double a0H = (AH + 1.0) - (AH - 1.0) * std::cos(w0H) + 2.0 * std::sqrt(AH) * alphaH;
        colH_b0 = (AH * ((AH + 1.0) + (AH - 1.0) * std::cos(w0H) + 2.0 * std::sqrt(AH) * alphaH)) / a0H;
        colH_b1 = (-2.0 * AH * ((AH - 1.0) + (AH + 1.0) * std::cos(w0H))) / a0H;
        colH_b2 = (AH * ((AH + 1.0) + (AH - 1.0) * std::cos(w0H) - 2.0 * std::sqrt(AH) * alphaH)) / a0H;
        colH_a1 = (2.0 * ((AH - 1.0) - (AH + 1.0) * std::cos(w0H))) / a0H;
        colH_a2 = ((AH + 1.0) - (AH - 1.0) * std::cos(w0H) - 2.0 * std::sqrt(AH) * alphaH) / a0H;

        ja_a = juce::jmap(cv, 0.0, 1.0, 20.0, 5.0);
        ja_k = juce::jmap(cv, 0.0, 1.0, 10.0, 20.0);
        lastColorParam = colorParam;
    }
    // Air (15k~30kHz Peak)
    if (airParam != lastAirParam) {
        double cv = static_cast<double>(airParam) / 100.0;
        double gainDb = cv * 4.0;
        double A = std::pow(10.0, gainDb / 40.0);
        double w0 = juce::MathConstants<double>::twoPi * std::min(22000.0, fs * 0.49) / fs;
        double alpha = std::sin(w0) / (2.0 * 0.5);
        double a0 = 1.0 + alpha / A;
        air_b0 = (1.0 + alpha * A) / a0;
        air_b1 = (-2.0 * std::cos(w0)) / a0;
        air_b2 = (1.0 - alpha * A) / a0;
        air_a1 = (-2.0 * std::cos(w0)) / a0;
        air_a2 = (1.0 - alpha / A) / a0;
        lastAirParam = airParam;
    }
    // Age
    if (ageParam != lastAgeParam) {
        double cv = static_cast<double>(ageParam) / 100.0;
        double fcHpf = std::min(juce::jmap(cv, 0.0, 1.0, 10.0, 45.0), fs * 0.49);
        alphaHpf = std::exp(-juce::MathConstants<double>::twoPi * fcHpf / fs);
        double fcLpf = std::min(juce::jmap(cv, 0.0, 1.0, 38000.0, 12000.0), fs * 0.49);
        alphaLpf = std::exp(-juce::MathConstants<double>::twoPi * fcLpf / fs);
        lastAgeParam = ageParam;
    }

    double dIn = static_cast<double>(input);

    // Hysteresis (Analyzer bypass)
    double hystOut = isAnalyzerMode ? dIn : hysteresisEngine.processSample(static_cast<float>(dIn), ja_a, ja_k, ja_c);

    // Color Low
    double colLOut = colL_b0 * hystOut + colL_b1 * colL_x1 + colL_b2 * colL_x2 - colL_a1 * colL_y1 - colL_a2 * colL_y2;
    colL_x2 = colL_x1; colL_x1 = hystOut; colL_y2 = colL_y1; colL_y1 = colLOut;

    // Color High
    double colHOut = colH_b0 * colLOut + colH_b1 * colH_x1 + colH_b2 * colH_x2 - colH_a1 * colH_y1 - colH_a2 * colH_y2;
    colH_x2 = colH_x1; colH_x1 = colLOut; colH_y2 = colH_y1; colH_y1 = colHOut;

    // Air Peak
    double airOut = air_b0 * colHOut + air_b1 * air_x1 + air_b2 * air_x2 - air_a1 * air_y1 - air_a2 * air_y2;
    air_x2 = air_x1; air_x1 = colHOut; air_y2 = air_y1; air_y1 = airOut;

    // Age
    double lpfOut = airOut * (1.0 - alphaLpf) + alphaLpf * lpfState; lpfState = lpfOut;
    double hpfOut = lpfOut - lastInput + alphaHpf * hpfState; hpfState = hpfOut; lastInput = lpfOut;

    return static_cast<float>(hpfOut);
}

float OutputTransformer_Cinemag::processDrySample(float input, float airParam, float ageParam) {
    double dIn = static_cast<double>(input);

    double colLOut = colL_b0 * dIn + colL_b1 * colL_x1_dry + colL_b2 * colL_x2_dry - colL_a1 * colL_y1_dry - colL_a2 * colL_y2_dry;
    colL_x2_dry = colL_x1_dry; colL_x1_dry = dIn; colL_y2_dry = colL_y1_dry; colL_y1_dry = colLOut;

    double colHOut = colH_b0 * colLOut + colH_b1 * colH_x1_dry + colH_b2 * colH_x2_dry - colH_a1 * colH_y1_dry - colH_a2 * colH_y2_dry;
    colH_x2_dry = colH_x1_dry; colH_x1_dry = colLOut; colH_y2_dry = colH_y1_dry; colH_y1_dry = colHOut;

    double airOut = air_b0 * colHOut + air_b1 * air_x1_dry + air_b2 * air_x2_dry - air_a1 * air_y1_dry - air_a2 * air_y2_dry;
    air_x2_dry = air_x1_dry; air_x1_dry = colHOut; air_y2_dry = air_y1_dry; air_y1_dry = airOut;

    double lpfOut = airOut * (1.0 - alphaLpf) + alphaLpf * lpfState_dry; lpfState_dry = lpfOut;
    double hpfOut = lpfOut - lastInput_dry + alphaHpf * hpfState_dry; hpfState_dry = hpfOut; lastInput_dry = lpfOut;

    return static_cast<float>(hpfOut);
}