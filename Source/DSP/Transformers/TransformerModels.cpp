#pragma execution_character_set("utf-8")
#include "TransformerModels.h"
#include <cmath>
#include <algorithm>

//==============================================================================
// Biquad Coefficient Calculators (Inline helpers)
//==============================================================================
static void calcHighShelf(double fs, double fc, double gainDb, double& b0, double& b1, double& b2, double& a1, double& a2) {
    if (std::abs(gainDb) < 0.01) { b0 = 1.0; b1 = 0.0; b2 = 0.0; a1 = 0.0; a2 = 0.0; return; }
    double A = std::pow(10.0, gainDb / 40.0);
    double w0 = juce::MathConstants<double>::twoPi * std::min(fc, fs * 0.49) / fs;
    double alpha = std::sin(w0) / 2.0 * std::sqrt((A + 1.0 / A) * (1.0 / 1.0 - 1.0) + 2.0); // Q=0.707 (S=1) -> sqrt(2)
    double a0 = (A + 1.0) - (A - 1.0) * std::cos(w0) + 2.0 * std::sqrt(A) * alpha;
    b0 = (A * ((A + 1.0) + (A - 1.0) * std::cos(w0) + 2.0 * std::sqrt(A) * alpha)) / a0;
    b1 = (-2.0 * A * ((A - 1.0) + (A + 1.0) * std::cos(w0))) / a0;
    b2 = (A * ((A + 1.0) + (A - 1.0) * std::cos(w0) - 2.0 * std::sqrt(A) * alpha)) / a0;
    a1 = (2.0 * ((A - 1.0) - (A + 1.0) * std::cos(w0))) / a0;
    a2 = ((A + 1.0) - (A - 1.0) * std::cos(w0) - 2.0 * std::sqrt(A) * alpha) / a0;
}

static void calcLowShelf(double fs, double fc, double gainDb, double& b0, double& b1, double& b2, double& a1, double& a2) {
    if (std::abs(gainDb) < 0.01) { b0 = 1.0; b1 = 0.0; b2 = 0.0; a1 = 0.0; a2 = 0.0; return; }
    double A = std::pow(10.0, gainDb / 40.0);
    double w0 = juce::MathConstants<double>::twoPi * std::min(fc, fs * 0.49) / fs;
    double alpha = std::sin(w0) / 2.0 * std::sqrt(2.0); // S=1
    double a0 = (A + 1.0) + (A - 1.0) * std::cos(w0) + 2.0 * std::sqrt(A) * alpha;
    b0 = (A * ((A + 1.0) - (A - 1.0) * std::cos(w0) + 2.0 * std::sqrt(A) * alpha)) / a0;
    b1 = (2.0 * A * ((A - 1.0) - (A + 1.0) * std::cos(w0))) / a0;
    b2 = (A * ((A + 1.0) - (A - 1.0) * std::cos(w0) - 2.0 * std::sqrt(A) * alpha)) / a0;
    a1 = (-2.0 * ((A - 1.0) + (A + 1.0) * std::cos(w0))) / a0;
    a2 = ((A + 1.0) + (A - 1.0) * std::cos(w0) - 2.0 * std::sqrt(A) * alpha) / a0;
}

//==============================================================================
// Input Transformers
//==============================================================================
void InputTransformer_Nickel::prepare(double sampleRate) { fs = sampleRate; lastInputADAA = 0.0; lastInputADAA_dry = 0.0; }
float InputTransformer_Nickel::processSample(float input) {
    double dIn = static_cast<double>(input); double dx = dIn - lastInputADAA; double out = 0.0;
    if (std::abs(dx) < 1e-8) out = ADAA_Math::fx_hardclip((dIn + lastInputADAA) * 0.5);
    else out = (ADAA_Math::F1_hardclip(dIn) - ADAA_Math::F1_hardclip(lastInputADAA)) / dx;
    lastInputADAA = dIn; return static_cast<float>(out);
}
float InputTransformer_Nickel::processDrySample(float input) {
    double dIn = static_cast<double>(input); double out = (dIn + lastInputADAA_dry) * 0.5;
    lastInputADAA_dry = dIn; return static_cast<float>(out);
}

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

void InputTransformer_Iron::prepare(double sampleRate) { fs = sampleRate; lastInputADAA = 0.0; lastInputADAA_dry = 0.0; }
float InputTransformer_Iron::processSample(float input) {
    double dIn = static_cast<double>(input); double dx = dIn - lastInputADAA; double out = 0.0;
    if (std::abs(dx) < 1e-8) out = ADAA_Math::fx_iron((dIn + lastInputADAA) * 0.5);
    else out = (ADAA_Math::F1_iron(dIn) - ADAA_Math::F1_iron(lastInputADAA)) / dx;
    lastInputADAA = dIn; return static_cast<float>(out);
}
float InputTransformer_Iron::processDrySample(float input) {
    double dIn = static_cast<double>(input); double out = (dIn + lastInputADAA_dry) * 0.5;
    lastInputADAA_dry = dIn; return static_cast<float>(out);
}

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
    double dIn = static_cast<double>(input); double out = (dIn + lastInputADAA_dry) * 0.5;
    lastInputADAA_dry = dIn; return static_cast<float>(out);
}

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
// Output Transformers
//==============================================================================

void OutputTransformer_Amorphous::prepare(double sampleRate) {
    fs = sampleRate;
    lastColorParam = -1.0f; lastAirParam = -1.0f; lastAgeParam = -1.0f;
    lpfState = 0.0; alphaLpf = 0.0; lpfState_dry = 0.0;
    air_x1 = 0.0; air_x2 = 0.0; air_y1 = 0.0; air_y2 = 0.0;
    air_x1_dry = 0.0; air_x2_dry = 0.0; air_y1_dry = 0.0; air_y2_dry = 0.0;
    air_b0 = 1.0; air_b1 = 0.0; air_b2 = 0.0; air_a1 = 0.0; air_a2 = 0.0;
}

float OutputTransformer_Amorphous::processSample(float input, float colorParam, float airParam, float ageParam) {
    if (airParam != lastAirParam) {
        double cv = static_cast<double>(airParam) / 100.0;
        calcHighShelf(fs, 5000.0, cv * 3.0, air_b0, air_b1, air_b2, air_a1, air_a2);
        lastAirParam = airParam;
    }
    if (ageParam != lastAgeParam) {
        double cv = static_cast<double>(ageParam) / 100.0;
        double fcLpf = std::min(juce::jmap(cv, 0.0, 1.0, 40000.0, 15000.0), fs * 0.49);
        alphaLpf = std::exp(-juce::MathConstants<double>::twoPi * fcLpf / fs);
        lastAgeParam = ageParam;
    }

    double dIn = static_cast<double>(input);

    // Saturation Bypass for Analyzer
    double hystOut = isAnalyzerMode ? dIn : std::tanh(dIn * 1.05);

    double airOut = air_b0 * hystOut + air_b1 * air_x1 + air_b2 * air_x2 - air_a1 * air_y1 - air_a2 * air_y2;
    air_x2 = air_x1; air_x1 = hystOut; air_y2 = air_y1; air_y1 = airOut;

    double lpfOut = airOut * (1.0 - alphaLpf) + alphaLpf * lpfState; lpfState = lpfOut;
    return static_cast<float>(lpfOut);
}

float OutputTransformer_Amorphous::processDrySample(float input, float airParam, float ageParam) {
    double dIn = static_cast<double>(input);
    double airOut = air_b0 * dIn + air_b1 * air_x1_dry + air_b2 * air_x2_dry - air_a1 * air_y1_dry - air_a2 * air_y2_dry;
    air_x2_dry = air_x1_dry; air_x1_dry = dIn; air_y2_dry = air_y1_dry; air_y1_dry = airOut;
    double lpfOut = airOut * (1.0 - alphaLpf) + alphaLpf * lpfState_dry; lpfState_dry = lpfOut;
    return static_cast<float>(lpfOut);
}

void OutputTransformer_Steel::prepare(double sampleRate) {
    fs = sampleRate;
    hpfState = 0.0; lastInput = 0.0; apfState = 0.0; lastApfInput = 0.0; lpfState = 0.0;
    hpfState_dry = 0.0; lastInput_dry = 0.0; apfState_dry = 0.0; lastApfInput_dry = 0.0; lpfState_dry = 0.0;
    lastColorParam = -1.0f; lastAirParam = -1.0f; lastAgeParam = -1.0f;
    hysteresisEngine.prepare();
}

float OutputTransformer_Steel::processSample(float input, float colorParam, float airParam, float ageParam) {
    if (colorParam != lastColorParam) {
        double cv = static_cast<double>(colorParam) / 100.0;
        hystDrive = juce::jmap(cv, 0.0, 1.0, 1.0, 3.5);
        hystHc = juce::jmap(cv, 0.0, 1.0, 0.1, 0.4);
        lastColorParam = colorParam;
    }
    if (ageParam != lastAgeParam) {
        double cv = static_cast<double>(ageParam) / 100.0;
        double fcHpf = std::min(juce::jmap(cv, 0.0, 1.0, 20.0, 60.0), fs * 0.49);
        alphaHpf = std::exp(-juce::MathConstants<double>::twoPi * fcHpf / fs);
        double fcLpf = std::min(juce::jmap(cv, 0.0, 1.0, 22050.0, 8000.0), fs * 0.49);
        alphaLpf = std::exp(-juce::MathConstants<double>::twoPi * fcLpf / fs);
        lastAgeParam = ageParam;
    }
    if (airParam != lastAirParam) {
        double cv = static_cast<double>(airParam) / 100.0;
        double delayPhase = juce::jmap(cv, 0.0, 1.0, 0.0, 0.8);
        apfAlpha = (1.0 - delayPhase) / (1.0 + delayPhase);
        lastAirParam = airParam;
    }

    double dIn = static_cast<double>(input);
    double hpfOut = dIn - lastInput + alphaHpf * hpfState; hpfState = hpfOut; lastInput = dIn;

    // Saturation Bypass for Analyzer
    double hystOut = isAnalyzerMode ? hpfOut : hysteresisEngine.processSample(static_cast<float>(hpfOut), hystDrive, hystHc);

    double apfOut = apfAlpha * hystOut + lastApfInput - apfAlpha * apfState; apfState = apfOut; lastApfInput = hystOut;
    double lpfOut = apfOut * (1.0 - alphaLpf) + alphaLpf * lpfState; lpfState = lpfOut;
    return static_cast<float>(lpfOut);
}

float OutputTransformer_Steel::processDrySample(float input, float airParam, float ageParam) {
    double dIn = static_cast<double>(input);
    double hpfOut = dIn - lastInput_dry + alphaHpf * hpfState_dry; hpfState_dry = hpfOut; lastInput_dry = dIn;
    double apfOut = apfAlpha * hpfOut + lastApfInput_dry - apfAlpha * apfState_dry; apfState_dry = apfOut; lastApfInput_dry = hpfOut;
    double lpfOut = apfOut * (1.0 - alphaLpf) + alphaLpf * lpfState_dry; lpfState_dry = lpfOut;
    return static_cast<float>(lpfOut);
}

void OutputTransformer_Iron::prepare(double sampleRate) {
    fs = sampleRate;
    hpfState = 0.0; lastInput = 0.0; lpfState = 0.0;
    hpfState_dry = 0.0; lastInput_dry = 0.0; lpfState_dry = 0.0;
    lastColorParam = -1.0f; lastAirParam = -1.0f; lastAgeParam = -1.0f;
    hysteresisEngine.prepare();
}

float OutputTransformer_Iron::processSample(float input, float colorParam, float airParam, float ageParam) {
    if (colorParam != lastColorParam) {
        double cv = static_cast<double>(colorParam) / 100.0;
        hystDrive = juce::jmap(cv, 0.0, 1.0, 1.0, 5.0);
        hystHc = juce::jmap(cv, 0.0, 1.0, 0.05, 0.25);
        lastColorParam = colorParam;
    }
    if (ageParam != lastAgeParam) {
        double cv = static_cast<double>(ageParam) / 100.0;
        double fcHpf = std::min(juce::jmap(cv, 0.0, 1.0, 10.0, 40.0), fs * 0.49);
        alphaHpf = std::exp(-juce::MathConstants<double>::twoPi * fcHpf / fs);
        double fcLpf = std::min(juce::jmap(cv, 0.0, 1.0, 24000.0, 10000.0), fs * 0.49);
        alphaLpf = std::exp(-juce::MathConstants<double>::twoPi * fcLpf / fs);
        lastAgeParam = ageParam;
    }

    double dIn = static_cast<double>(input);
    double hpfOut = dIn - lastInput + alphaHpf * hpfState; hpfState = hpfOut; lastInput = dIn;

    // Saturation Bypass for Analyzer
    double hystOut = isAnalyzerMode ? hpfOut : hysteresisEngine.processSample(static_cast<float>(hpfOut), hystDrive, hystHc);

    double lpfOut = hystOut * (1.0 - alphaLpf) + alphaLpf * lpfState; lpfState = lpfOut;
    return static_cast<float>(lpfOut);
}

float OutputTransformer_Iron::processDrySample(float input, float airParam, float ageParam) {
    double dIn = static_cast<double>(input);
    double hpfOut = dIn - lastInput_dry + alphaHpf * hpfState_dry; hpfState_dry = hpfOut; lastInput_dry = dIn;
    double lpfOut = hpfOut * (1.0 - alphaLpf) + alphaLpf * lpfState_dry; lpfState_dry = lpfOut;
    return static_cast<float>(lpfOut);
}

void OutputTransformer_Nickel::prepare(double sampleRate) {
    fs = sampleRate;
    hpfState = 0.0; lastInput = 0.0; lpfState = 0.0;
    hpfState_dry = 0.0; lastInput_dry = 0.0; lpfState_dry = 0.0;
    lastColorParam = -1.0f; lastAirParam = -1.0f; lastAgeParam = -1.0f;
    hysteresisEngine.prepare();
}

float OutputTransformer_Nickel::processSample(float input, float colorParam, float airParam, float ageParam) {
    if (colorParam != lastColorParam) {
        double cv = static_cast<double>(colorParam) / 100.0;
        ja_a = juce::jmap(cv, 0.0, 1.0, 4.0, 1.0);
        ja_k = juce::jmap(cv, 0.0, 1.0, 0.0001, 0.01);
        lastColorParam = colorParam;
    }
    if (ageParam != lastAgeParam) {
        double cv = static_cast<double>(ageParam) / 100.0;
        double fcHpf = std::min(juce::jmap(cv, 0.0, 1.0, 5.0, 20.0), fs * 0.49);
        alphaHpf = std::exp(-juce::MathConstants<double>::twoPi * fcHpf / fs);
        double fcLpf = std::min(juce::jmap(cv, 0.0, 1.0, 30000.0, 15000.0), fs * 0.49);
        alphaLpf = std::exp(-juce::MathConstants<double>::twoPi * fcLpf / fs);
        lastAgeParam = ageParam;
    }

    double dIn = static_cast<double>(input);
    double hpfOut = dIn - lastInput + alphaHpf * hpfState; hpfState = hpfOut; lastInput = dIn;

    // Saturation Bypass for Analyzer
    double hystOut = isAnalyzerMode ? hpfOut : hysteresisEngine.processSample(static_cast<float>(hpfOut), ja_a, ja_k, ja_c) * 1.5;

    double lpfOut = hystOut * (1.0 - alphaLpf) + alphaLpf * lpfState; lpfState = lpfOut;
    return static_cast<float>(lpfOut);
}

float OutputTransformer_Nickel::processDrySample(float input, float airParam, float ageParam) {
    double dIn = static_cast<double>(input);
    double hpfOut = dIn - lastInput_dry + alphaHpf * hpfState_dry; hpfState_dry = hpfOut; lastInput_dry = dIn;
    double lpfOut = hpfOut * (1.0 - alphaLpf) + alphaLpf * lpfState_dry; lpfState_dry = lpfOut;
    return static_cast<float>(lpfOut);
}

//==============================================================================
// Carnhill (TG2 Style) - Unity gain safe saturation + Biquad EQ
//==============================================================================
void OutputTransformer_Carnhill::prepare(double sampleRate) {
    fs = sampleRate;
    lastColorParam = -1.0f; lastAirParam = -1.0f; lastAgeParam = -1.0f;
    hpfState = 0.0; lpfState = 0.0; lastInput = 0.0; alphaHpf = 0.0; alphaLpf = 0.0;
    hpfState_dry = 0.0; lpfState_dry = 0.0; lastInput_dry = 0.0;
    col_x1 = 0.0; col_x2 = 0.0; col_y1 = 0.0; col_y2 = 0.0;
    col_x1_dry = 0.0; col_x2_dry = 0.0; col_y1_dry = 0.0; col_y2_dry = 0.0;
    col_b0 = 1.0; col_b1 = 0.0; col_b2 = 0.0; col_a1 = 0.0; col_a2 = 0.0;
    air_x1 = 0.0; air_x2 = 0.0; air_y1 = 0.0; air_y2 = 0.0;
    air_x1_dry = 0.0; air_x2_dry = 0.0; air_y1_dry = 0.0; air_y2_dry = 0.0;
    air_b0 = 1.0; air_b1 = 0.0; air_b2 = 0.0; air_a1 = 0.0; air_a2 = 0.0;
}

float OutputTransformer_Carnhill::processSample(float input, float colorParam, float airParam, float ageParam) {
    if (colorParam != lastColorParam) {
        double cv = static_cast<double>(colorParam) / 100.0;
        calcHighShelf(fs, 3500.0, cv * 2.5, col_b0, col_b1, col_b2, col_a1, col_a2);
        lastColorParam = colorParam;
    }
    if (airParam != lastAirParam) {
        double cv = static_cast<double>(airParam) / 100.0;
        calcHighShelf(fs, 10000.0, cv * 2.0, air_b0, air_b1, air_b2, air_a1, air_a2);
        lastAirParam = airParam;
    }
    if (ageParam != lastAgeParam || alphaHpf == 0.0) { // 初期化安全策
        double cv = static_cast<double>(ageParam) / 100.0;
        double fcHpf = std::min(juce::jmap(cv, 0.0, 1.0, 10.0, 45.0), fs * 0.49);
        alphaHpf = std::exp(-juce::MathConstants<double>::twoPi * fcHpf / fs);
        double fcLpf = std::min(juce::jmap(cv, 0.0, 1.0, 32000.0, 12000.0), fs * 0.49);
        alphaLpf = std::exp(-juce::MathConstants<double>::twoPi * fcLpf / fs);
        lastAgeParam = ageParam;
    }

    double dIn = static_cast<double>(input);

    // Saturation Bypass for Analyzer. Carnhill = Asymmetric Soft Clip (Even Harmonics)
    double hystOut = isAnalyzerMode ? dIn : std::tanh(dIn + 0.1 * dIn * dIn);

    double colOut = col_b0 * hystOut + col_b1 * col_x1 + col_b2 * col_x2 - col_a1 * col_y1 - col_a2 * col_y2;
    col_x2 = col_x1; col_x1 = hystOut; col_y2 = col_y1; col_y1 = colOut;

    double airOut = air_b0 * colOut + air_b1 * air_x1 + air_b2 * air_x2 - air_a1 * air_y1 - air_a2 * air_y2;
    air_x2 = air_x1; air_x1 = colOut; air_y2 = air_y1; air_y1 = airOut;

    double lpfOut = airOut * (1.0 - alphaLpf) + alphaLpf * lpfState; lpfState = lpfOut;
    double hpfOut = lpfOut - lastInput + alphaHpf * hpfState; hpfState = hpfOut; lastInput = lpfOut;

    return static_cast<float>(hpfOut);
}

float OutputTransformer_Carnhill::processDrySample(float input, float airParam, float ageParam) {
    double dIn = static_cast<double>(input);
    double colOut = col_b0 * dIn + col_b1 * col_x1_dry + col_b2 * col_x2_dry - col_a1 * col_y1_dry - col_a2 * col_y2_dry;
    col_x2_dry = col_x1_dry; col_x1_dry = dIn; col_y2_dry = col_y1_dry; col_y1_dry = colOut;

    double airOut = air_b0 * colOut + air_b1 * air_x1_dry + air_b2 * air_x2_dry - air_a1 * air_y1_dry - air_a2 * air_y2_dry;
    air_x2_dry = air_x1_dry; air_x1_dry = colOut; air_y2_dry = air_y1_dry; air_y1_dry = airOut;

    double lpfOut = airOut * (1.0 - alphaLpf) + alphaLpf * lpfState_dry; lpfState_dry = lpfOut;
    double hpfOut = lpfOut - lastInput_dry + alphaHpf * hpfState_dry; hpfState_dry = hpfOut; lastInput_dry = lpfOut;

    return static_cast<float>(hpfOut);
}

//==============================================================================
// Cinemag (B173 Style) - Unity gain safe saturation + Biquad EQ
//==============================================================================
void OutputTransformer_Cinemag::prepare(double sampleRate) {
    fs = sampleRate;
    lastColorParam = -1.0f; lastAirParam = -1.0f; lastAgeParam = -1.0f;
    hpfState = 0.0; lpfState = 0.0; lastInput = 0.0; alphaHpf = 0.0; alphaLpf = 0.0;
    hpfState_dry = 0.0; lpfState_dry = 0.0; lastInput_dry = 0.0;
    colL_x1 = 0.0; colL_x2 = 0.0; colL_y1 = 0.0; colL_y2 = 0.0;
    colL_x1_dry = 0.0; colL_x2_dry = 0.0; colL_y1_dry = 0.0; colL_y2_dry = 0.0;
    colL_b0 = 1.0; colL_b1 = 0.0; colL_b2 = 0.0; colL_a1 = 0.0; colL_a2 = 0.0;
    colH_x1 = 0.0; colH_x2 = 0.0; colH_y1 = 0.0; colH_y2 = 0.0;
    colH_x1_dry = 0.0; colH_x2_dry = 0.0; colH_y1_dry = 0.0; colH_y2_dry = 0.0;
    colH_b0 = 1.0; colH_b1 = 0.0; colH_b2 = 0.0; colH_a1 = 0.0; colH_a2 = 0.0;
    air_x1 = 0.0; air_x2 = 0.0; air_y1 = 0.0; air_y2 = 0.0;
    air_x1_dry = 0.0; air_x2_dry = 0.0; air_y1_dry = 0.0; air_y2_dry = 0.0;
    air_b0 = 1.0; air_b1 = 0.0; air_b2 = 0.0; air_a1 = 0.0; air_a2 = 0.0;
}

float OutputTransformer_Cinemag::processSample(float input, float colorParam, float airParam, float ageParam) {
    if (colorParam != lastColorParam) {
        double cv = static_cast<double>(colorParam) / 100.0;
        calcLowShelf(fs, 80.0, cv * 1.5, colL_b0, colL_b1, colL_b2, colL_a1, colL_a2);
        calcHighShelf(fs, 12000.0, cv * 1.5, colH_b0, colH_b1, colH_b2, colH_a1, colH_a2);
        lastColorParam = colorParam;
    }
    if (airParam != lastAirParam) {
        double cv = static_cast<double>(airParam) / 100.0;
        calcHighShelf(fs, 5000.0, cv * 3.0, air_b0, air_b1, air_b2, air_a1, air_a2); // Gentle High Shelf
        lastAirParam = airParam;
    }
    if (ageParam != lastAgeParam || alphaHpf == 0.0) {
        double cv = static_cast<double>(ageParam) / 100.0;
        double fcHpf = std::min(juce::jmap(cv, 0.0, 1.0, 10.0, 45.0), fs * 0.49);
        alphaHpf = std::exp(-juce::MathConstants<double>::twoPi * fcHpf / fs);
        double fcLpf = std::min(juce::jmap(cv, 0.0, 1.0, 38000.0, 12000.0), fs * 0.49);
        alphaLpf = std::exp(-juce::MathConstants<double>::twoPi * fcLpf / fs);
        lastAgeParam = ageParam;
    }

    double dIn = static_cast<double>(input);

    // Saturation Bypass for Analyzer. Cinemag = Transparent Soft Clip
    double hystOut = isAnalyzerMode ? dIn : std::tanh(dIn);

    double colLOut = colL_b0 * hystOut + colL_b1 * colL_x1 + colL_b2 * colL_x2 - colL_a1 * colL_y1 - colL_a2 * colL_y2;
    colL_x2 = colL_x1; colL_x1 = hystOut; colL_y2 = colL_y1; colL_y1 = colLOut;

    double colHOut = colH_b0 * colLOut + colH_b1 * colH_x1 + colH_b2 * colH_x2 - colH_a1 * colH_y1 - colH_a2 * colH_y2;
    colH_x2 = colH_x1; colH_x1 = colLOut; colH_y2 = colH_y1; colH_y1 = colHOut;

    double airOut = air_b0 * colHOut + air_b1 * air_x1 + air_b2 * air_x2 - air_a1 * air_y1 - air_a2 * air_y2;
    air_x2 = air_x1; air_x1 = colHOut; air_y2 = air_y1; air_y1 = airOut;

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