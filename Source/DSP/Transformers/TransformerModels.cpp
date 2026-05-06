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
    double out = (dIn + lastInputADAA_dry) * 0.5; // ADAA1 Linear Moving Average
    lastInputADAA_dry = dIn; return static_cast<float>(out);
}

//==============================================================================
// Input: Steel
//==============================================================================
void InputTransformer_Steel::prepare(double sampleRate) {
    fs = sampleRate; lastInputADAA = 0.0; hpfState = 0.0; lastInput = 0.0;
    lastInputADAA_dry = 0.0; hpfState_dry = 0.0; lastInput_dry = 0.0;
    alphaHpf = std::exp(-juce::MathConstants<double>::twoPi * 15.0 / fs);
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
    double out = (hpfOut + lastInputADAA_dry) * 0.5; // ADAA1 Linear Moving Average
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
        double A = std::pow(10.0, gainDb / 40.0); double w0 = juce::MathConstants<double>::twoPi * 20000.0 / fs;
        double alphaQ = std::sin(w0) / (2.0 * 0.707); double a0 = 1.0 + alphaQ / A;
        b0 = (1.0 + alphaQ * A) / a0; b1 = (-2.0 * std::cos(w0)) / a0; b2 = (1.0 - alphaQ * A) / a0;
        a1 = (-2.0 * std::cos(w0)) / a0; a2 = (1.0 - alphaQ / A) / a0; lastAirParam = airParam;
    }
    if (ageParam != lastAgeParam) {
        double fcLpf = juce::jmap(static_cast<double>(ageParam), 0.0, 100.0, 35000.0, 12000.0);
        alphaLpf = std::exp(-juce::MathConstants<double>::twoPi * fcLpf / fs); lastAgeParam = ageParam;
    }

    double out = static_cast<double>(input);
    if (!isAnalyzerMode) { out *= driveGain; out = std::clamp(out, -threshold, threshold); out /= driveGain; }

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
        double fcHpf = juce::jmap(cv, 0.0, 1.0, 15.0, 5.0); alphaHpf = std::exp(-juce::MathConstants<double>::twoPi * fcHpf / fs);
        double apfFreq = juce::jmap(cv, 0.0, 1.0, 60.0, 20.0); double tanVal = std::tan(juce::MathConstants<double>::pi * apfFreq / fs);
        apfAlpha = (tanVal - 1.0) / (tanVal + 1.0);
        hystHc = juce::jmap(cv, 0.0, 1.0, 0.001, 0.30); hystDrive = juce::jmap(cv, 0.0, 1.0, 1.0, 2.5);
        lastColorParam = colorParam;
    }
    if (airParam != lastAirParam) {
        double gainDb = juce::jmap(static_cast<double>(airParam), 0.0, 100.0, 0.0, 6.0);
        double A = std::pow(10.0, gainDb / 40.0); double w0 = juce::MathConstants<double>::twoPi * 15000.0 / fs;
        double alphaQ = std::sin(w0) / (2.0 * 0.707); double a0 = 1.0 + alphaQ / A;
        b0 = (1.0 + alphaQ * A) / a0; b1 = (-2.0 * std::cos(w0)) / a0; b2 = (1.0 - alphaQ * A) / a0;
        a1 = (-2.0 * std::cos(w0)) / a0; a2 = (1.0 - alphaQ / A) / a0; lastAirParam = airParam;
    }
    if (ageParam != lastAgeParam) {
        double fcLpf = juce::jmap(static_cast<double>(ageParam), 0.0, 100.0, 30000.0, 10000.0);
        alphaLpf = std::exp(-juce::MathConstants<double>::twoPi * fcLpf / fs); lastAgeParam = ageParam;
    }

    double dIn = static_cast<double>(input);
    double apfOut = apfAlpha * dIn + lastApfInput - apfAlpha * apfState; apfState = apfOut; lastApfInput = dIn;
    double hystOut = isAnalyzerMode ? static_cast<double>(apfOut) : hysteresisEngine.processSample(static_cast<float>(apfOut), hystDrive, hystHc);
    double airOut = b0 * hystOut + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    x2 = x1; x1 = hystOut; y2 = y1; y1 = airOut;
    double lpfOut = airOut * (1.0 - alphaLpf) + alphaLpf * lpfState; lpfState = lpfOut;
    double hpfOut = lpfOut - lastInput + alphaHpf * hpfState; hpfState = hpfOut; lastInput = lpfOut;
    return static_cast<float>(hpfOut);
}
float OutputTransformer_Steel::processDrySample(float input, float airParam, float ageParam) {
    double dIn = static_cast<double>(input);
    double apfOut = apfAlpha * dIn + lastApfInput_dry - apfAlpha * apfState_dry; apfState_dry = apfOut; lastApfInput_dry = dIn;
    // Hysteresis Bypass
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
        double fcHpf = juce::jmap(cv, 0.0, 1.0, 20.0, 10.0); alphaHpf = std::exp(-juce::MathConstants<double>::twoPi * fcHpf / fs);
        hystHc = juce::jmap(cv, 0.0, 1.0, 0.001, 0.30); hystDrive = juce::jmap(cv, 0.0, 1.0, 1.0, 2.5);
        lastColorParam = colorParam;
    }
    if (airParam != lastAirParam) {
        double gainDb = juce::jmap(static_cast<double>(airParam), 0.0, 100.0, 0.0, 6.0);
        double A = std::pow(10.0, gainDb / 40.0); double w0 = juce::MathConstants<double>::twoPi * 15000.0 / fs;
        double alphaQ = std::sin(w0) / (2.0 * 0.707); double a0 = 1.0 + alphaQ / A;
        b0 = (1.0 + alphaQ * A) / a0; b1 = (-2.0 * std::cos(w0)) / a0; b2 = (1.0 - alphaQ * A) / a0;
        a1 = (-2.0 * std::cos(w0)) / a0; a2 = (1.0 - alphaQ / A) / a0; lastAirParam = airParam;
    }
    if (ageParam != lastAgeParam) {
        double fcLpf = juce::jmap(static_cast<double>(ageParam), 0.0, 100.0, 30000.0, 10000.0);
        alphaLpf = std::exp(-juce::MathConstants<double>::twoPi * fcLpf / fs); lastAgeParam = ageParam;
    }
    double dIn = static_cast<double>(input);
    double hystOut = isAnalyzerMode ? dIn : hysteresisEngine.processSample(static_cast<float>(dIn), hystDrive, hystHc);
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
        double fcHpf = juce::jmap(cv, 0.0, 1.0, 5.0, 2.0); alphaHpf = std::exp(-juce::MathConstants<double>::twoPi * fcHpf / fs);
        ja_a = juce::jmap(cv, 0.0, 1.0, 30.0, 0.05); ja_k = juce::jmap(cv, 0.0, 1.0, 0.00001, 0.04); ja_c = juce::jmap(cv, 0.0, 1.0, 0.9999, 0.45);
        lastColorParam = colorParam;
    }
    if (airParam != lastAirParam) {
        double gainDb = juce::jmap(static_cast<double>(airParam), 0.0, 100.0, 0.0, 6.0);
        double A = std::pow(10.0, gainDb / 40.0); double w0 = juce::MathConstants<double>::twoPi * 15000.0 / fs;
        double alphaQ = std::sin(w0) / (2.0 * 0.707); double a0 = 1.0 + alphaQ / A;
        b0 = (1.0 + alphaQ * A) / a0; b1 = (-2.0 * std::cos(w0)) / a0; b2 = (1.0 - alphaQ * A) / a0;
        a1 = (-2.0 * std::cos(w0)) / a0; a2 = (1.0 - alphaQ / A) / a0; lastAirParam = airParam;
    }
    if (ageParam != lastAgeParam) {
        double fcLpf = juce::jmap(static_cast<double>(ageParam), 0.0, 100.0, 35000.0, 10000.0);
        alphaLpf = std::exp(-juce::MathConstants<double>::twoPi * fcLpf / fs); lastAgeParam = ageParam;
    }
    double dIn = static_cast<double>(input); double hystOut = dIn;
    if (!isAnalyzerMode) {
        double cv = std::pow(static_cast<double>(colorParam) / 100.0, 3.0); double df = juce::jmap(cv, 0.0, 1.0, 1.0, 2.5);
        double sc = 3.0 * ja_a;
        hystOut = hysteresisEngine.processSample(static_cast<float>(dIn * sc * df), ja_a, ja_k, ja_c);
        hystOut /= std::sqrt(df);
    }
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

//==============================================================================
// Input: Carnhill (TG2用)
//==============================================================================
void InputTransformer_Carnhill::prepare(double sampleRate) { fs = sampleRate; lastInputADAA = 0.0; lastInputADAA_dry = 0.0; }
float InputTransformer_Carnhill::processSample(float input) {
    double dIn = static_cast<double>(input); double dx = dIn - lastInputADAA; double out = 0.0;
    if (std::abs(dx) < 1e-8) out = ADAA_Math::fx_iron((dIn + lastInputADAA) * 0.5);
    else out = (ADAA_Math::F1_iron(dIn) - ADAA_Math::F1_iron(lastInputADAA)) / dx;
    lastInputADAA = dIn; return static_cast<float>(out);
}
float InputTransformer_Carnhill::processDrySample(float input) {
    double dIn = static_cast<double>(input);
    double out = (dIn + lastInputADAA_dry) * 0.5;
    lastInputADAA_dry = dIn; return static_cast<float>(out);
}

//==============================================================================
// Input: Cinemag (B173用)
//==============================================================================
void InputTransformer_Cinemag::prepare(double sampleRate) { fs = sampleRate; lastInputADAA = 0.0; lastInputADAA_dry = 0.0; }
float InputTransformer_Cinemag::processSample(float input) {
    double dIn = static_cast<double>(input); double dx = dIn - lastInputADAA; double out = 0.0;
    if (std::abs(dx) < 1e-8) out = ADAA_Math::fx_hardclip((dIn + lastInputADAA) * 0.5);
    else out = (ADAA_Math::F1_hardclip(dIn) - ADAA_Math::F1_hardclip(lastInputADAA)) / dx;
    lastInputADAA = dIn; return static_cast<float>(out);
}
float InputTransformer_Cinemag::processDrySample(float input) {
    double dIn = static_cast<double>(input);
    double out = (dIn + lastInputADAA_dry) * 0.5;
    lastInputADAA_dry = dIn; return static_cast<float>(out);
}

//==============================================================================
// Output: Carnhill (TG2用)
//==============================================================================
void OutputTransformer_Carnhill::prepare(double sampleRate) {
    fs = sampleRate;
    hpfState = 0.0; lastInput = 0.0; lpfState = 0.0; x1 = 0.0; x2 = 0.0; y1 = 0.0; y2 = 0.0;
    hpfState_dry = 0.0; lastInput_dry = 0.0; lpfState_dry = 0.0; x1_dry = 0.0; x2_dry = 0.0; y1_dry = 0.0; y2_dry = 0.0;
    lastColorParam = -1.0f; lastAirParam = -1.0f; lastAgeParam = -1.0f; hysteresisEngine.prepare();
}
float OutputTransformer_Carnhill::processSample(float input, float colorParam, float airParam, float ageParam) {
    if (colorParam != lastColorParam) {
        double cv = std::pow(static_cast<double>(colorParam) / 100.0, 3.0);
        double fcHpf = juce::jmap(cv, 0.0, 1.0, 25.0, 10.0); alphaHpf = std::exp(-juce::MathConstants<double>::twoPi * fcHpf / fs);
        ja_a = juce::jmap(cv, 0.0, 1.0, 35.0, 10.0); ja_k = juce::jmap(cv, 0.0, 1.0, 15.0, 50.0);
        lastColorParam = colorParam;
    }
    if (airParam != lastAirParam) {
        double gainDb = juce::jmap(static_cast<double>(airParam), 0.0, 100.0, 0.0, 4.5);
        double A = std::pow(10.0, gainDb / 40.0); double w0 = juce::MathConstants<double>::twoPi * 25000.0 / fs;
        double alphaQ = std::sin(w0) / (2.0 * 0.5); double a0 = 1.0 + alphaQ / A;
        b0 = (1.0 + alphaQ * A) / a0; b1 = (-2.0 * std::cos(w0)) / a0; b2 = (1.0 - alphaQ * A) / a0;
        a1 = (-2.0 * std::cos(w0)) / a0; a2 = (1.0 - alphaQ / A) / a0; lastAirParam = airParam;
    }
    if (ageParam != lastAgeParam) {
        double fcLpf = juce::jmap(static_cast<double>(ageParam), 0.0, 100.0, 32000.0, 12000.0);
        alphaLpf = std::exp(-juce::MathConstants<double>::twoPi * fcLpf / fs); lastAgeParam = ageParam;
    }
    double dIn = static_cast<double>(input); double hystOut = dIn;
    if (!isAnalyzerMode) {
        hystOut = hysteresisEngine.processSample(static_cast<float>(dIn * 2.0), ja_a, ja_k, ja_c);
        hystOut *= 0.5;
    }
    double airOut = b0 * hystOut + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    x2 = x1; x1 = hystOut; y2 = y1; y1 = airOut;
    double lpfOut = airOut * (1.0 - alphaLpf) + alphaLpf * lpfState; lpfState = lpfOut;
    double hpfOut = lpfOut - lastInput + alphaHpf * hpfState; hpfState = hpfOut; lastInput = lpfOut;
    return static_cast<float>(hpfOut);
}
float OutputTransformer_Carnhill::processDrySample(float input, float airParam, float ageParam) {
    double dIn = static_cast<double>(input);
    double airOut = b0 * dIn + b1 * x1_dry + b2 * x2_dry - a1 * y1_dry - a2 * y2_dry;
    x2_dry = x1_dry; x1_dry = dIn; y2_dry = y1_dry; y1_dry = airOut;
    double lpfOut = airOut * (1.0 - alphaLpf) + alphaLpf * lpfState_dry; lpfState_dry = lpfOut;
    double hpfOut = lpfOut - lastInput_dry + alphaHpf * hpfState_dry; hpfState_dry = hpfOut; lastInput_dry = lpfOut;
    return static_cast<float>(hpfOut);
}

//==============================================================================
// Output: Cinemag (B173用)
//==============================================================================
void OutputTransformer_Cinemag::prepare(double sampleRate) {
    fs = sampleRate;
    hpfState = 0.0; lastInput = 0.0; lpfState = 0.0; x1 = 0.0; x2 = 0.0; y1 = 0.0; y2 = 0.0;
    hpfState_dry = 0.0; lastInput_dry = 0.0; lpfState_dry = 0.0; x1_dry = 0.0; x2_dry = 0.0; y1_dry = 0.0; y2_dry = 0.0;
    lastColorParam = -1.0f; lastAirParam = -1.0f; lastAgeParam = -1.0f; hysteresisEngine.prepare();
}
float OutputTransformer_Cinemag::processSample(float input, float colorParam, float airParam, float ageParam) {
    if (colorParam != lastColorParam) {
        double cv = std::pow(static_cast<double>(colorParam) / 100.0, 3.0);
        double fcHpf = juce::jmap(cv, 0.0, 1.0, 15.0, 5.0); alphaHpf = std::exp(-juce::MathConstants<double>::twoPi * fcHpf / fs);
        ja_a = juce::jmap(cv, 0.0, 1.0, 550.0, 200.0); ja_k = juce::jmap(cv, 0.0, 1.0, 420.0, 600.0);
        lastColorParam = colorParam;
    }
    if (airParam != lastAirParam) {
        double gainDb = juce::jmap(static_cast<double>(airParam), 0.0, 100.0, 0.0, 5.5);
        double A = std::pow(10.0, gainDb / 40.0); double w0 = juce::MathConstants<double>::twoPi * 18000.0 / fs;
        double alphaQ = std::sin(w0) / (2.0 * 0.707); double a0 = 1.0 + alphaQ / A;
        b0 = (1.0 + alphaQ * A) / a0; b1 = (-2.0 * std::cos(w0)) / a0; b2 = (1.0 - alphaQ * A) / a0;
        a1 = (-2.0 * std::cos(w0)) / a0; a2 = (1.0 - alphaQ / A) / a0; lastAirParam = airParam;
    }
    if (ageParam != lastAgeParam) {
        double fcLpf = juce::jmap(static_cast<double>(ageParam), 0.0, 100.0, 38000.0, 15000.0);
        alphaLpf = std::exp(-juce::MathConstants<double>::twoPi * fcLpf / fs); lastAgeParam = ageParam;
    }
    double dIn = static_cast<double>(input); double hystOut = dIn;
    if (!isAnalyzerMode) {
        hystOut = hysteresisEngine.processSample(static_cast<float>(dIn), ja_a, ja_k, ja_c);
    }
    double airOut = b0 * hystOut + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    x2 = x1; x1 = hystOut; y2 = y1; y1 = airOut;
    double lpfOut = airOut * (1.0 - alphaLpf) + alphaLpf * lpfState; lpfState = lpfOut;
    double hpfOut = lpfOut - lastInput + alphaHpf * hpfState; hpfState = hpfOut; lastInput = lpfOut;
    return static_cast<float>(hpfOut);
}
float OutputTransformer_Cinemag::processDrySample(float input, float airParam, float ageParam) {
    double dIn = static_cast<double>(input);
    double airOut = b0 * dIn + b1 * x1_dry + b2 * x2_dry - a1 * y1_dry - a2 * y2_dry;
    x2_dry = x1_dry; x1_dry = dIn; y2_dry = y1_dry; y1_dry = airOut;
    double lpfOut = airOut * (1.0 - alphaLpf) + alphaLpf * lpfState_dry; lpfState_dry = lpfOut;
    double hpfOut = lpfOut - lastInput_dry + alphaHpf * hpfState_dry; hpfState_dry = hpfOut; lastInput_dry = lpfOut;
    return static_cast<float>(hpfOut);
}