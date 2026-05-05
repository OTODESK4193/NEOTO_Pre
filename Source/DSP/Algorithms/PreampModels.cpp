#include "PreampModels.h"
#include <cmath>
#include <algorithm>

//==============================================================================
// 1. API Style
//==============================================================================
void Preamp_API::prepare(double sampleRate) {
    fs = sampleRate;
    integratorState = 0.0;
    lastInputADAA = 0.0;
    lastInputADAA_dry = 0.0;
    lastSoftclipOut = 0.0;
    envState = 0.0;
    lastDriveParam = -1.0f; lastCharParam = -1.0f; lastAsymParam = -1.0f; lastAgeParam = -1.0f;
}

float Preamp_API::processDrySample(float input, float driveParam, float charParam, float asymParam, float ageParam) {
    double dInput = static_cast<double>(input);
    double out = (dInput + lastInputADAA_dry) * 0.5; // ADAA1 Linear Moving Average
    lastInputADAA_dry = dInput;
    return static_cast<float>(out);
}

float Preamp_API::processSample(float input, float driveParam, float charParam, float asymParam, float ageParam)
{
    if (driveParam != lastDriveParam) {
        double normalizedDrive = static_cast<double>(driveParam) / 100.0;
        double driveDb = std::pow(normalizedDrive, 3.0) * 24.0;
        gain = juce::Decibels::decibelsToGain(driveDb);
        makeUp = 1.0 / (1.0 + (gain - 1.0) * 0.15);
        lastDriveParam = driveParam;
    }

    if (charParam != lastCharParam) {
        double charNormalized = static_cast<double>(charParam) / 100.0;
        mixEven = charNormalized;
        mixOdd = 1.0 - charNormalized;
        double fc = 35.0;
        alpha = std::exp(-juce::MathConstants<double>::twoPi * fc / fs);
        oneMinusAlpha = 1.0 - alpha;
        envAttackCoef = std::exp(-1.0 / (0.01 * fs));
        envReleaseCoef = std::exp(-1.0 / (0.1 * fs));
        lastCharParam = charParam;
    }

    if (asymParam != lastAsymParam) {
        bias = juce::jmap(static_cast<double>(asymParam), 0.0, 100.0, 0.0, 0.4);
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

    double intOut = drivenSignal * oneMinusAlpha + alpha * integratorState;
    integratorState = intOut;

    double x = intOut + bias;
    double softclipOut = 0.0;
    double dx = x - lastInputADAA;
    double abs_dx = std::abs(dx);

    auto compute_fallback = [&](double val) -> double {
        return (ADAA_Math::fx_chebyshev_even(val) * mixEven) + (ADAA_Math::fx_chebyshev_odd(val) * mixOdd);
        };

    auto compute_adaa = [&](double val_cur, double val_prev, double delta) -> double {
        double adaa_even = (ADAA_Math::F1_chebyshev_even(val_cur) - ADAA_Math::F1_chebyshev_even(val_prev)) / delta;
        double adaa_odd = (ADAA_Math::F1_chebyshev_odd(val_cur) - ADAA_Math::F1_chebyshev_odd(val_prev)) / delta;
        return (adaa_even * mixEven) + (adaa_odd * mixOdd);
        };

    if (abs_dx < 1e-8) { softclipOut = compute_fallback((x + lastInputADAA) * 0.5); }
    else { softclipOut = compute_adaa(x, lastInputADAA, dx); }

    lastInputADAA = x;
    double outputWithoutDC = softclipOut - fxBias;
    double diffOut = (outputWithoutDC - alpha * lastSoftclipOut) / oneMinusAlpha;
    lastSoftclipOut = outputWithoutDC;

    return static_cast<float>(diffOut * makeUp);
}

//==============================================================================
// 2. Neve Style (1073)
//==============================================================================
void Preamp_Neve::prepare(double sampleRate) {
    fs = sampleRate;
    integratorState = 0.0; lastInputADAA = 0.0; lastInputADAA_dry = 0.0; lastSoftclipOut = 0.0; envState = 0.0;
    lastDriveParam = -1.0f; lastCharParam = -1.0f; lastAsymParam = -1.0f; lastAgeParam = -1.0f;
}

float Preamp_Neve::processDrySample(float input, float driveParam, float charParam, float asymParam, float ageParam) {
    double dInput = static_cast<double>(input);
    double out = (dInput + lastInputADAA_dry) * 0.5;
    lastInputADAA_dry = dInput;
    return static_cast<float>(out);
}

float Preamp_Neve::processSample(float input, float driveParam, float charParam, float asymParam, float ageParam)
{
    if (driveParam != lastDriveParam) {
        double normalizedDrive = static_cast<double>(driveParam) / 100.0;
        double driveDb = std::pow(normalizedDrive, 2.0) * 20.0; // Neveは徐々に歪むソフトニー
        gain = juce::Decibels::decibelsToGain(driveDb);
        makeUp = 1.0 / (1.0 + (gain - 1.0) * 0.2);
        lastDriveParam = driveParam;
    }

    if (charParam != lastCharParam) {
        double charNormalized = static_cast<double>(charParam) / 100.0;
        mixEven = 0.6 + (charNormalized * 0.4); // 偶数次主体
        mixOdd = 1.0 - mixEven;
        double fc = 50.0; // 低域の膨らみをAPIより少し高めに設定
        alpha = std::exp(-juce::MathConstants<double>::twoPi * fc / fs);
        oneMinusAlpha = 1.0 - alpha;
        envAttackCoef = std::exp(-1.0 / (0.005 * fs));
        envReleaseCoef = std::exp(-1.0 / (0.05 * fs));
        lastCharParam = charParam;
    }

    if (asymParam != lastAsymParam) {
        // Class A の DC偏磁をシミュレート
        bias = juce::jmap(static_cast<double>(asymParam), 0.0, 100.0, 0.1, 0.6);
        fxBias = (ADAA_Math::fx_chebyshev_even(bias) * mixEven) + (ADAA_Math::fx_chebyshev_odd(bias) * mixOdd);
        lastAsymParam = asymParam;
    }

    if (ageParam != lastAgeParam) {
        sagRatio = juce::jmap(static_cast<double>(ageParam), 0.0, 100.0, 0.0, 0.3); // トランジェント丸め込み強め
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

    double x = intOut + bias;
    double softclipOut = 0.0;
    double dx = x - lastInputADAA;
    double abs_dx = std::abs(dx);

    auto compute_fallback = [&](double val) -> double {
        return (ADAA_Math::fx_chebyshev_even(val) * mixEven) + (ADAA_Math::fx_chebyshev_odd(val) * mixOdd);
        };

    auto compute_adaa = [&](double val_cur, double val_prev, double delta) -> double {
        double adaa_even = (ADAA_Math::F1_chebyshev_even(val_cur) - ADAA_Math::F1_chebyshev_even(val_prev)) / delta;
        double adaa_odd = (ADAA_Math::F1_chebyshev_odd(val_cur) - ADAA_Math::F1_chebyshev_odd(val_prev)) / delta;
        return (adaa_even * mixEven) + (adaa_odd * mixOdd);
        };

    if (abs_dx < 1e-8) { softclipOut = compute_fallback((x + lastInputADAA) * 0.5); }
    else { softclipOut = compute_adaa(x, lastInputADAA, dx); }

    lastInputADAA = x;
    double outputWithoutDC = softclipOut - fxBias;
    double diffOut = (outputWithoutDC - alpha * lastSoftclipOut) / oneMinusAlpha;
    lastSoftclipOut = outputWithoutDC;

    return static_cast<float>(diffOut * makeUp);
}

//==============================================================================
// 3. Vintage Tube Style (V76s)
//==============================================================================
void Preamp_Tube::prepare(double sampleRate) {
    fs = sampleRate;
    integratorState = 0.0; lastInputADAA = 0.0; lastInputADAA_dry = 0.0; lastSoftclipOut = 0.0; envState = 0.0;
    hpfState = 0.0; lastInput = 0.0; hpfState_dry = 0.0; lastInput_dry = 0.0;
    lastDriveParam = -1.0f; lastCharParam = -1.0f; lastAsymParam = -1.0f; lastAgeParam = -1.0f;
}

float Preamp_Tube::processDrySample(float input, float driveParam, float charParam, float asymParam, float ageParam) {
    double dInput = static_cast<double>(input);
    double hpfOut = dInput - lastInput_dry + alphaHpf * hpfState_dry;
    hpfState_dry = hpfOut; lastInput_dry = dInput;
    double out = (hpfOut + lastInputADAA_dry) * 0.5;
    lastInputADAA_dry = hpfOut;
    return static_cast<float>(out);
}

float Preamp_Tube::processSample(float input, float driveParam, float charParam, float asymParam, float ageParam)
{
    if (driveParam != lastDriveParam) {
        double normalizedDrive = static_cast<double>(driveParam) / 100.0;
        double driveDb = std::pow(normalizedDrive, 4.0) * 30.0; // NFB限界突破で急激に歪む
        gain = juce::Decibels::decibelsToGain(driveDb);
        makeUp = 1.0 / (1.0 + (gain - 1.0) * 0.1);
        lastDriveParam = driveParam;
    }

    if (charParam != lastCharParam) {
        double charNormalized = static_cast<double>(charParam) / 100.0;
        mixEven = 0.3 + (charNormalized * 0.4);
        mixOdd = 1.0 - mixEven;
        double fc_int = 25.0;
        alpha = std::exp(-juce::MathConstants<double>::twoPi * fc_int / fs);
        oneMinusAlpha = 1.0 - alpha;
        double fc_hpf = 30.0; // V76s特有の低域位相先行 (Phase Lead)
        alphaHpf = std::exp(-juce::MathConstants<double>::twoPi * fc_hpf / fs);
        envAttackCoef = std::exp(-1.0 / (0.02 * fs));
        envReleaseCoef = std::exp(-1.0 / (0.15 * fs));
        lastCharParam = charParam;
    }

    if (asymParam != lastAsymParam) {
        bias = juce::jmap(static_cast<double>(asymParam), 0.0, 100.0, 0.0, 0.5);
        fxBias = ADAA_Math::fx_tube(bias);
        lastAsymParam = asymParam;
    }

    if (ageParam != lastAgeParam) {
        sagRatio = juce::jmap(static_cast<double>(ageParam), 0.0, 100.0, 0.0, 0.4); // アイランド効果による強いサグ
        lastAgeParam = ageParam;
    }

    double dIn = static_cast<double>(input);
    double hpfOut = dIn - lastInput + alphaHpf * hpfState;
    hpfState = hpfOut; lastInput = dIn;

    double drivenSignal = hpfOut * gain;
    double absIn = std::abs(drivenSignal);

    if (absIn > envState) envState = envAttackCoef * envState + (1.0 - envAttackCoef) * absIn;
    else envState = envReleaseCoef * envState + (1.0 - envReleaseCoef) * absIn;

    double dynamicGain = 1.0 - (envState * sagRatio);
    drivenSignal *= std::max(0.1, dynamicGain);

    double intOut = drivenSignal * oneMinusAlpha + alpha * integratorState;
    integratorState = intOut;

    double x = intOut + bias;
    double softclipOut = 0.0;
    double dx = x - lastInputADAA;
    double abs_dx = std::abs(dx);

    if (abs_dx < 1e-8) { softclipOut = ADAA_Math::fx_tube((x + lastInputADAA) * 0.5); }
    else { softclipOut = (ADAA_Math::F1_tube(x) - ADAA_Math::F1_tube(lastInputADAA)) / dx; }

    lastInputADAA = x;
    double outputWithoutDC = softclipOut - fxBias;
    double diffOut = (outputWithoutDC - alpha * lastSoftclipOut) / oneMinusAlpha;
    lastSoftclipOut = outputWithoutDC;

    return static_cast<float>(diffOut * makeUp);
}

//==============================================================================
// 4. SSL Modern Style
//==============================================================================
void Preamp_SSL::prepare(double sampleRate) {
    fs = sampleRate;
    lastInputADAA = 0.0; lastInputADAA_dry = 0.0;
    lastDriveParam = -1.0f; lastCharParam = -1.0f; lastAsymParam = -1.0f; lastAgeParam = -1.0f;
}

float Preamp_SSL::processDrySample(float input, float driveParam, float charParam, float asymParam, float ageParam) {
    double dInput = static_cast<double>(input);
    double out = (dInput + lastInputADAA_dry) * 0.5;
    lastInputADAA_dry = dInput;
    return static_cast<float>(out);
}

float Preamp_SSL::processSample(float input, float driveParam, float charParam, float asymParam, float ageParam)
{
    if (driveParam != lastDriveParam) {
        double normalizedDrive = static_cast<double>(driveParam) / 100.0;
        double driveDb = std::pow(normalizedDrive, 2.0) * 15.0; // 限界までクリーン
        gain = juce::Decibels::decibelsToGain(driveDb);
        makeUp = 1.0 / (1.0 + (gain - 1.0) * 0.2);
        lastDriveParam = driveParam;
    }

    if (asymParam != lastAsymParam) {
        bias = juce::jmap(static_cast<double>(asymParam), 0.0, 100.0, 0.0, 0.2); // SSLは対称性が高い
        fxBias = ADAA_Math::fx_hardknee(bias);
        lastAsymParam = asymParam;
    }

    double drivenSignal = static_cast<double>(input) * gain;
    double x = drivenSignal + bias;
    double softclipOut = 0.0;
    double dx = x - lastInputADAA;
    double abs_dx = std::abs(dx);

    if (abs_dx < 1e-8) { softclipOut = ADAA_Math::fx_hardknee((x + lastInputADAA) * 0.5); }
    else { softclipOut = (ADAA_Math::F1_hardknee(x) - ADAA_Math::F1_hardknee(lastInputADAA)) / dx; }

    lastInputADAA = x;
    double outputWithoutDC = softclipOut - fxBias;

    return static_cast<float>(outputWithoutDC * makeUp);
}