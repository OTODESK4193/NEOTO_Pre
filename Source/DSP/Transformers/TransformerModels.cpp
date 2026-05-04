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

// ==============================================================================
// ★ Output: Amorphous の実装
// ==============================================================================
void OutputTransformer_Amorphous::prepare(double sampleRate) {
    fs = sampleRate;
    x1 = 0.0; x2 = 0.0; y1 = 0.0; y2 = 0.0; lpfState = 0.0;
    x1_dry = 0.0; x2_dry = 0.0; y1_dry = 0.0; y2_dry = 0.0; lpfState_dry = 0.0; // ★ Dry
    lastColorParam = -1.0f; lastAirParam = -1.0f; lastAgeParam = -1.0f;
}

float OutputTransformer_Amorphous::processSample(float input, float colorParam, float airParam, float ageParam) {
    if (colorParam != lastColorParam) {
        double curve = std::pow(static_cast<double>(colorParam) / 100.0, 3.0);
        threshold = juce::jmap(curve, 0.0, 1.0, 5.0, 0.35);
        driveGain = juce::jmap(curve, 0.0, 1.0, 1.0, 3.5);
        lastColorParam = colorParam;
    }
    if (airParam != lastAirParam) {
        double gainDb = juce::jmap(static_cast<double>(airParam), 0.0, 100.0, 0.0, 6.0);
        double A = std::pow(10.0, gainDb / 40.0);
        double w0 = juce::MathConstants<double>::twoPi * 20000.0 / fs;
        double alphaQ = std::sin(w0) / (2.0 * 0.707);
        double a0 = 1.0 + alphaQ / A;
        b0 = (1.0 + alphaQ * A) / a0; b1 = (-2.0 * std::cos(w0)) / a0; b2 = (1.0 - alphaQ * A) / a0;
        a1 = (-2.0 * std::cos(w0)) / a0; a2 = (1.0 - alphaQ / A) / a0;
        lastAirParam = airParam;
    }
    if (ageParam != lastAgeParam) {
        double fcLpf = juce::jmap(static_cast<double>(ageParam), 0.0, 100.0, 35000.0, 12000.0);
        alphaLpf = std::exp(-juce::MathConstants<double>::twoPi * fcLpf / fs);
        lastAgeParam = ageParam;
    }

    double dIn = static_cast<double>(input);
    double out = dIn;

    if (!isAnalyzerMode) {
        out *= driveGain;
        out = std::clamp(out, -threshold, threshold);
        out /= driveGain;
    }

    double airOut = b0 * out + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    x2 = x1; x1 = out; y2 = y1; y1 = airOut;

    double lpfOut = airOut * (1.0 - alphaLpf) + alphaLpf * lpfState;
    lpfState = lpfOut;

    return static_cast<float>(lpfOut);
}

// ★ Dry用プロセス (IIRフィルタのみ適用)
float OutputTransformer_Amorphous::processDrySample(float input, float airParam, float ageParam) {
    double dIn = static_cast<double>(input);

    double airOut = b0 * dIn + b1 * x1_dry + b2 * x2_dry - a1 * y1_dry - a2 * y2_dry;
    x2_dry = x1_dry; x1_dry = dIn; y2_dry = y1_dry; y1_dry = airOut;

    double lpfOut = airOut * (1.0 - alphaLpf) + alphaLpf * lpfState_dry;
    lpfState_dry = lpfOut;

    return static_cast<float>(lpfOut);
}

//==============================================================================
// Output: Steel の実装
//==============================================================================
void OutputTransformer_Steel::prepare(double sampleRate) {
    fs = sampleRate;
    hpfState = 0.0; lastInput = 0.0; apfState = 0.0; lastApfInput = 0.0; lpfState = 0.0;
    x1 = 0.0; x2 = 0.0; y1 = 0.0; y2 = 0.0;
    // ★ Dry
    hpfState_dry = 0.0; lastInput_dry = 0.0; apfState_dry = 0.0; lastApfInput_dry = 0.0; lpfState_dry = 0.0;
    x1_dry = 0.0; x2_dry = 0.0; y1_dry = 0.0; y2_dry = 0.0;

    lastColorParam = -1.0f; lastAirParam = -1.0f; lastAgeParam = -1.0f;
    hysteresisEngine.prepare();
}

float OutputTransformer_Steel::processSample(float input, float colorParam, float airParam, float ageParam) {
    if (colorParam != lastColorParam) {
        double curve = std::pow(static_cast<double>(colorParam) / 100.0, 3.0);
        double fcHpf = juce::jmap(curve, 0.0, 1.0, 15.0, 5.0);
        alphaHpf = std::exp(-juce::MathConstants<double>::twoPi * fcHpf / fs);
        double apfFreq = juce::jmap(curve, 0.0, 1.0, 60.0, 20.0);
        double tanVal = std::tan(juce::MathConstants<double>::pi * apfFreq / fs);
        apfAlpha = (tanVal - 1.0) / (tanVal + 1.0);
        hystHc = juce::jmap(curve, 0.0, 1.0, 0.001, 0.30);
        hystDrive = juce::jmap(curve, 0.0, 1.0, 1.0, 2.5);
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
    double apfOut = apfAlpha * dIn + lastApfInput - apfAlpha * apfState;
    apfState = apfOut; lastApfInput = dIn;

    double hystOut = isAnalyzerMode ? static_cast<double>(apfOut)
        : hysteresisEngine.processSample(static_cast<float>(apfOut), hystDrive, hystHc);

    double airOut = b0 * hystOut + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    x2 = x1; x1 = hystOut; y2 = y1; y1 = airOut;

    double lpfOut = airOut * (1.0 - alphaLpf) + alphaLpf * lpfState;
    lpfState = lpfOut;

    double hpfOut = lpfOut - lastInput + alphaHpf * hpfState;
    hpfState = hpfOut; lastInput = lpfOut;

    return static_cast<float>(hpfOut);
}

// ★ Dry用プロセス (ヒステリシスをスキップし、APF/Air/LPF/HPFの位相回転のみを適用)
float OutputTransformer_Steel::processDrySample(float input, float airParam, float ageParam) {
    double dIn = static_cast<double>(input);

    double apfOut = apfAlpha * dIn + lastApfInput_dry - apfAlpha * apfState_dry;
    apfState_dry = apfOut; lastApfInput_dry = dIn;

    // ヒステリシスをスキップ
    double airOut = b0 * apfOut + b1 * x1_dry + b2 * x2_dry - a1 * y1_dry - a2 * y2_dry;
    x2_dry = x1_dry; x1_dry = apfOut; y2_dry = y1_dry; y1_dry = airOut;

    double lpfOut = airOut * (1.0 - alphaLpf) + alphaLpf * lpfState_dry;
    lpfState_dry = lpfOut;

    double hpfOut = lpfOut - lastInput_dry + alphaHpf * hpfState_dry;
    hpfState_dry = hpfOut; lastInput_dry = lpfOut;

    return static_cast<float>(hpfOut);
}

//==============================================================================
// Output: Iron の実装
//==============================================================================
void OutputTransformer_Iron::prepare(double sampleRate) {
    fs = sampleRate;
    hpfState = 0.0; lastInput = 0.0; lpfState = 0.0;
    x1 = 0.0; x2 = 0.0; y1 = 0.0; y2 = 0.0;
    // ★ Dry
    hpfState_dry = 0.0; lastInput_dry = 0.0; lpfState_dry = 0.0;
    x1_dry = 0.0; x2_dry = 0.0; y1_dry = 0.0; y2_dry = 0.0;

    lastColorParam = -1.0f; lastAirParam = -1.0f; lastAgeParam = -1.0f;
    hysteresisEngine.prepare();
}

float OutputTransformer_Iron::processSample(float input, float colorParam, float airParam, float ageParam) {
    if (colorParam != lastColorParam) {
        double curve = std::pow(static_cast<double>(colorParam) / 100.0, 3.0);
        double fcHpf = juce::jmap(curve, 0.0, 1.0, 20.0, 10.0);
        alphaHpf = std::exp(-juce::MathConstants<double>::twoPi * fcHpf / fs);
        hystHc = juce::jmap(curve, 0.0, 1.0, 0.001, 0.30);
        hystDrive = juce::jmap(curve, 0.0, 1.0, 1.0, 2.5);
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
    double hystOut = isAnalyzerMode ? static_cast<double>(dIn)
        : hysteresisEngine.processSample(static_cast<float>(dIn), hystDrive, hystHc);

    double airOut = b0 * hystOut + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    x2 = x1; x1 = hystOut; y2 = y1; y1 = airOut;

    double lpfOut = airOut * (1.0 - alphaLpf) + alphaLpf * lpfState;
    lpfState = lpfOut;

    double hpfOut = lpfOut - lastInput + alphaHpf * hpfState;
    hpfState = hpfOut; lastInput = lpfOut;

    return static_cast<float>(hpfOut);
}

// ★ Dry用プロセス
float OutputTransformer_Iron::processDrySample(float input, float airParam, float ageParam) {
    double dIn = static_cast<double>(input);

    double airOut = b0 * dIn + b1 * x1_dry + b2 * x2_dry - a1 * y1_dry - a2 * y2_dry;
    x2_dry = x1_dry; x1_dry = dIn; y2_dry = y1_dry; y1_dry = airOut;

    double lpfOut = airOut * (1.0 - alphaLpf) + alphaLpf * lpfState_dry;
    lpfState_dry = lpfOut;

    double hpfOut = lpfOut - lastInput_dry + alphaHpf * hpfState_dry;
    hpfState_dry = hpfOut; lastInput_dry = lpfOut;

    return static_cast<float>(hpfOut);
}

//==============================================================================
// Output: Nickel の実装
//==============================================================================
void OutputTransformer_Nickel::prepare(double sampleRate) {
    fs = sampleRate;
    hpfState = 0.0; lastInput = 0.0; lpfState = 0.0;
    x1 = 0.0; x2 = 0.0; y1 = 0.0; y2 = 0.0;
    // ★ Dry
    hpfState_dry = 0.0; lastInput_dry = 0.0; lpfState_dry = 0.0;
    x1_dry = 0.0; x2_dry = 0.0; y1_dry = 0.0; y2_dry = 0.0;

    lastColorParam = -1.0f; lastAirParam = -1.0f; lastAgeParam = -1.0f;
    hysteresisEngine.prepare();
}

float OutputTransformer_Nickel::processSample(float input, float colorParam, float airParam, float ageParam) {
    if (colorParam != lastColorParam) {
        double curve = std::pow(static_cast<double>(colorParam) / 100.0, 3.0);
        double fcHpf = juce::jmap(curve, 0.0, 1.0, 5.0, 2.0);
        alphaHpf = std::exp(-juce::MathConstants<double>::twoPi * fcHpf / fs);
        ja_a = juce::jmap(curve, 0.0, 1.0, 30.0, 0.05);
        ja_k = juce::jmap(curve, 0.0, 1.0, 0.00001, 0.04);
        ja_c = juce::jmap(curve, 0.0, 1.0, 0.9999, 0.45);
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
        double fcLpf = juce::jmap(static_cast<double>(ageParam), 0.0, 100.0, 35000.0, 10000.0);
        alphaLpf = std::exp(-juce::MathConstants<double>::twoPi * fcLpf / fs);
        lastAgeParam = ageParam;
    }

    double dIn = static_cast<double>(input);
    double hystOut = dIn;

    if (!isAnalyzerMode) {
        double curve = std::pow(static_cast<double>(colorParam) / 100.0, 3.0);
        double driveFactor = juce::jmap(curve, 0.0, 1.0, 1.0, 2.5);
        double scaling = 3.0 * ja_a;
        hystOut = hysteresisEngine.processSample(static_cast<float>(dIn * scaling * driveFactor), ja_a, ja_k, ja_c);
        hystOut /= std::sqrt(driveFactor);
    }

    double airOut = b0 * hystOut + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    x2 = x1; x1 = hystOut; y2 = y1; y1 = airOut;

    double lpfOut = airOut * (1.0 - alphaLpf) + alphaLpf * lpfState;
    lpfState = lpfOut;

    double hpfOut = lpfOut - lastInput + alphaHpf * hpfState;
    hpfState = hpfOut; lastInput = lpfOut;

    return static_cast<float>(hpfOut);
}

// ★ Dry用プロセス
float OutputTransformer_Nickel::processDrySample(float input, float airParam, float ageParam) {
    double dIn = static_cast<double>(input);

    double airOut = b0 * dIn + b1 * x1_dry + b2 * x2_dry - a1 * y1_dry - a2 * y2_dry;
    x2_dry = x1_dry; x1_dry = dIn; y2_dry = y1_dry; y1_dry = airOut;

    double lpfOut = airOut * (1.0 - alphaLpf) + alphaLpf * lpfState_dry;
    lpfState_dry = lpfOut;

    double hpfOut = lpfOut - lastInput_dry + alphaHpf * hpfState_dry;
    hpfState_dry = hpfOut; lastInput_dry = lpfOut;

    return static_cast<float>(hpfOut);
}