#pragma execution_character_set("utf-8")
#include "PreampModels.h"
#include <cmath>
#include <algorithm>
#include <limits>

// ADAA2用の極小閾値
constexpr double ADAA_EPS = 1e-6;

//==============================================================================
// 1. API Style
//==============================================================================
void Preamp_API::prepare(double sampleRate) {
    fs = sampleRate;
    integratorState = 0.0;
    lastInputADAA1 = 0.0;
    lastInputADAA2 = 0.0;
    lastInputADAA_dry1 = 0.0;
    lastInputADAA_dry2 = 0.0;

    // API APF
    double wc = juce::MathConstants<double>::twoPi * 60.0 / fs;
    apfCoef = (std::tan(wc / 2.0) - 1.0) / (std::tan(wc / 2.0) + 1.0);
    apfStateIn = 0.0;
    apfStateOut = 0.0;
    apfStateIn_dry = 0.0;
    apfStateOut_dry = 0.0;

    // API アナログ特性のフィルタ (10Hz HPF / 22kHz LPF)
    double wc_hpf = juce::MathConstants<double>::twoPi * 10.0 / fs;
    alphaHpf = std::exp(-wc_hpf);
    double wc_lpf = juce::MathConstants<double>::twoPi * 22000.0 / fs;
    alphaLpf = std::exp(-wc_lpf);

    hpfState = 0.0; lastInput = 0.0;
    lpfState = 0.0;
    hpfState_dry = 0.0; lastInput_dry = 0.0;
    lpfState_dry = 0.0;

    lastSoftclipOut = 0.0;
    envState = 0.0;
    lastDriveParam = -1.0f; lastCharParam = -1.0f; lastAsymParam = -1.0f; lastAgeParam = -1.0f;
}

float Preamp_API::processDrySample(float input, float driveParam, float charParam, float asymParam, float ageParam) {
    double dInput = static_cast<double>(input);

    // ★ アナログ特性 (HPF + LPF)
    double hpfOut = dInput - lastInput_dry + alphaHpf * hpfState_dry;
    hpfState_dry = hpfOut; lastInput_dry = dInput;

    double lpfOut = hpfOut * (1.0 - alphaLpf) + alphaLpf * lpfState_dry;
    lpfState_dry = lpfOut;

    // ADAA2の線形遅延 (1.0 sample) を3タップ移動平均でシミュレート
    double out = (lpfOut + lastInputADAA_dry1 + lastInputADAA_dry2) / 3.0;
    lastInputADAA_dry2 = lastInputADAA_dry1;
    lastInputADAA_dry1 = lpfOut;

    // APFの位相推移を適用
    double apfOut = apfCoef * out + apfStateIn_dry - apfCoef * apfStateOut_dry;
    apfStateIn_dry = out;
    apfStateOut_dry = apfOut;

    return static_cast<float>(apfOut);
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

    double dIn = static_cast<double>(input);

    // ★ アナログ特性 (HPF + LPF)
    double hpfOut = dIn - lastInput + alphaHpf * hpfState;
    hpfState = hpfOut; lastInput = dIn;

    double lpfOut = hpfOut * (1.0 - alphaLpf) + alphaLpf * lpfState;
    lpfState = lpfOut;

    double drivenSignal = lpfOut * gain;
    double absIn = std::abs(drivenSignal);

    if (absIn > envState) envState = envAttackCoef * envState + (1.0 - envAttackCoef) * absIn;
    else envState = envReleaseCoef * envState + (1.0 - envReleaseCoef) * absIn;

    double dynamicGain = 1.0 - (envState * sagRatio);
    drivenSignal *= std::max(0.1, dynamicGain);

    double intOut = drivenSignal * oneMinusAlpha + alpha * integratorState;
    integratorState = intOut;

    double x = intOut + bias;
    double softclipOut = 0.0;

    // ★ アナライザーモードまたはDrive=0の場合は非線形処理をバイパス
    if (isAnalyzerMode || driveParam == 0.0f) {
        softclipOut = x;
        lastInputADAA2 = lastInputADAA1;
        lastInputADAA1 = x;
    }
    else {
        double diff1 = x - lastInputADAA1;
        double diff2 = lastInputADAA1 - lastInputADAA2;
        double diff_total = x - lastInputADAA2;

        auto calcFx = [&](double val) -> double {
            return (ADAA_Math::fx_chebyshev_even(val) * mixEven) + (ADAA_Math::fx_chebyshev_odd(val) * mixOdd);
            };
        auto calcF1 = [&](double val) -> double {
            return (ADAA_Math::F1_chebyshev_even(val) * mixEven) + (ADAA_Math::F1_chebyshev_odd(val) * mixOdd);
            };
        auto calcF2 = [&](double val) -> double {
            return (ADAA_Math::F2_chebyshev_even(val) * mixEven) + (ADAA_Math::F2_chebyshev_odd(val) * mixOdd);
            };

        // ★ ADAA2 フォールバックロジック
        if (std::abs(diff_total) <= ADAA_EPS) {
            softclipOut = calcFx((x + lastInputADAA2) * 0.5);
        }
        else if (std::abs(diff1) <= ADAA_EPS) {
            double div2 = (calcF2(lastInputADAA1) - calcF2(lastInputADAA2)) / diff2;
            softclipOut = (2.0 / diff_total) * (calcF1((x + lastInputADAA1) * 0.5) - div2);
        }
        else if (std::abs(diff2) <= ADAA_EPS) {
            double div1 = (calcF2(x) - calcF2(lastInputADAA1)) / diff1;
            softclipOut = (2.0 / diff_total) * (div1 - calcF1((lastInputADAA1 + lastInputADAA2) * 0.5));
        }
        else {
            double div1 = (calcF2(x) - calcF2(lastInputADAA1)) / diff1;
            double div2 = (calcF2(lastInputADAA1) - calcF2(lastInputADAA2)) / diff2;
            softclipOut = (2.0 / diff_total) * (div1 - div2);
        }

        // ★ ADAA2 ノッチ補償フィルタ
        double compensatedOut = softclipOut - 0.12 * lastInputADAA_dry2;
        softclipOut = compensatedOut;

        lastInputADAA2 = lastInputADAA1;
        lastInputADAA1 = x;
    }

    double outputWithoutDC = softclipOut - fxBias;

    // APF
    double apfIn = outputWithoutDC;
    double apfOut = apfCoef * apfIn + apfStateIn - apfCoef * apfStateOut;
    apfStateIn = apfIn;
    apfStateOut = apfOut;

    double diffOut = (apfOut - alpha * lastSoftclipOut) / oneMinusAlpha;
    lastSoftclipOut = apfOut;

    return static_cast<float>(diffOut * makeUp);
}

//==============================================================================
// 2. Neve Style (1073)
//==============================================================================
void Preamp_Neve::prepare(double sampleRate) {
    fs = sampleRate;
    integratorState = 0.0; lastInputADAA1 = 0.0; lastInputADAA2 = 0.0;
    lastInputADAA_dry1 = 0.0; lastInputADAA_dry2 = 0.0; lastSoftclipOut = 0.0; envState = 0.0;

    // Neve APF
    double wc = juce::MathConstants<double>::twoPi * 80.0 / fs;
    apfCoef = (std::tan(wc / 2.0) - 1.0) / (std::tan(wc / 2.0) + 1.0);
    apfStateIn = 0.0;
    apfStateOut = 0.0;
    apfStateIn_dry = 0.0;
    apfStateOut_dry = 0.0;

    lastDriveParam = -1.0f; lastCharParam = -1.0f; lastAsymParam = -1.0f; lastAgeParam = -1.0f;
}

float Preamp_Neve::processDrySample(float input, float driveParam, float charParam, float asymParam, float ageParam) {
    double dInput = static_cast<double>(input);
    double out = (dInput + lastInputADAA_dry1 + lastInputADAA_dry2) / 3.0;
    lastInputADAA_dry2 = lastInputADAA_dry1;
    lastInputADAA_dry1 = dInput;

    double apfOut = apfCoef * out + apfStateIn_dry - apfCoef * apfStateOut_dry;
    apfStateIn_dry = out;
    apfStateOut_dry = apfOut;

    return static_cast<float>(apfOut);
}

float Preamp_Neve::processSample(float input, float driveParam, float charParam, float asymParam, float ageParam)
{
    if (driveParam != lastDriveParam) {
        double normalizedDrive = static_cast<double>(driveParam) / 100.0;
        double driveDb = std::pow(normalizedDrive, 3.0) * 24.0;
        gain = juce::Decibels::decibelsToGain(driveDb);
        makeUp = 1.0 / (1.0 + (gain - 1.0) * 0.2);
        lastDriveParam = driveParam;
    }

    if (charParam != lastCharParam) {
        double charNormalized = static_cast<double>(charParam) / 100.0;
        mixEven = 0.6 + (charNormalized * 0.4);
        mixOdd = 1.0 - mixEven;
        double fc = 50.0;
        alpha = std::exp(-juce::MathConstants<double>::twoPi * fc / fs);
        oneMinusAlpha = 1.0 - alpha;
        envAttackCoef = std::exp(-1.0 / (0.005 * fs));
        envReleaseCoef = std::exp(-1.0 / (0.05 * fs));
        lastCharParam = charParam;
    }

    if (asymParam != lastAsymParam) {
        bias = juce::jmap(static_cast<double>(asymParam), 0.0, 100.0, 0.1, 0.6);
        fxBias = (ADAA_Math::fx_chebyshev_even(bias) * mixEven) + (ADAA_Math::fx_chebyshev_odd(bias) * mixOdd);
        lastAsymParam = asymParam;
    }

    if (ageParam != lastAgeParam) {
        sagRatio = juce::jmap(static_cast<double>(ageParam), 0.0, 100.0, 0.0, 0.3);
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

    if (isAnalyzerMode || driveParam == 0.0f) {
        softclipOut = x;
        lastInputADAA2 = lastInputADAA1;
        lastInputADAA1 = x;
    }
    else {
        double diff1 = x - lastInputADAA1;
        double diff2 = lastInputADAA1 - lastInputADAA2;
        double diff_total = x - lastInputADAA2;

        auto calcFx = [&](double val) -> double {
            return (ADAA_Math::fx_chebyshev_even(val) * mixEven) + (ADAA_Math::fx_chebyshev_odd(val) * mixOdd);
            };
        auto calcF1 = [&](double val) -> double {
            return (ADAA_Math::F1_chebyshev_even(val) * mixEven) + (ADAA_Math::F1_chebyshev_odd(val) * mixOdd);
            };
        auto calcF2 = [&](double val) -> double {
            return (ADAA_Math::F2_chebyshev_even(val) * mixEven) + (ADAA_Math::F2_chebyshev_odd(val) * mixOdd);
            };

        if (std::abs(diff_total) <= ADAA_EPS) {
            softclipOut = calcFx((x + lastInputADAA2) * 0.5);
        }
        else if (std::abs(diff1) <= ADAA_EPS) {
            double div2 = (calcF2(lastInputADAA1) - calcF2(lastInputADAA2)) / diff2;
            softclipOut = (2.0 / diff_total) * (calcF1((x + lastInputADAA1) * 0.5) - div2);
        }
        else if (std::abs(diff2) <= ADAA_EPS) {
            double div1 = (calcF2(x) - calcF2(lastInputADAA1)) / diff1;
            softclipOut = (2.0 / diff_total) * (div1 - calcF1((lastInputADAA1 + lastInputADAA2) * 0.5));
        }
        else {
            double div1 = (calcF2(x) - calcF2(lastInputADAA1)) / diff1;
            double div2 = (calcF2(lastInputADAA1) - calcF2(lastInputADAA2)) / diff2;
            softclipOut = (2.0 / diff_total) * (div1 - div2);
        }

        // ★ ADAA2 ノッチ補償フィルタ
        double compensatedOut = softclipOut - 0.12 * lastInputADAA_dry2;
        softclipOut = compensatedOut;

        lastInputADAA2 = lastInputADAA1;
        lastInputADAA1 = x;
    }

    double outputWithoutDC = softclipOut - fxBias;

    double apfIn = outputWithoutDC;
    double apfOut = apfCoef * apfIn + apfStateIn - apfCoef * apfStateOut;
    apfStateIn = apfIn;
    apfStateOut = apfOut;

    double diffOut = (apfOut - alpha * lastSoftclipOut) / oneMinusAlpha;
    lastSoftclipOut = apfOut;

    return static_cast<float>(diffOut * makeUp);
}

//==============================================================================
// 3. Vintage Tube Style (V76s)
//==============================================================================
void Preamp_Tube::prepare(double sampleRate) {
    fs = sampleRate;
    integratorState = 0.0; lastInputADAA1 = 0.0; lastInputADAA2 = 0.0;
    lastInputADAA_dry1 = 0.0; lastInputADAA_dry2 = 0.0; lastSoftclipOut = 0.0; envState = 0.0;
    hpfState = 0.0; lastInput = 0.0; hpfState_dry = 0.0; lastInput_dry = 0.0;

    // Tube APF
    double wc = juce::MathConstants<double>::twoPi * 40.0 / fs;
    apfCoef = (std::tan(wc / 2.0) - 1.0) / (std::tan(wc / 2.0) + 1.0);
    apfStateIn = 0.0;
    apfStateOut = 0.0;
    apfStateIn_dry = 0.0;
    apfStateOut_dry = 0.0;

    lastDriveParam = -1.0f; lastCharParam = -1.0f; lastAsymParam = -1.0f; lastAgeParam = -1.0f;
}

float Preamp_Tube::processDrySample(float input, float driveParam, float charParam, float asymParam, float ageParam) {
    double dInput = static_cast<double>(input);

    if (charParam != lastCharParam) {
        double fc_hpf = 30.0;
        alphaHpf = std::exp(-juce::MathConstants<double>::twoPi * fc_hpf / fs);
    }

    double hpfOut = dInput - lastInput_dry + alphaHpf * hpfState_dry;
    hpfState_dry = hpfOut; lastInput_dry = dInput;

    double out = (hpfOut + lastInputADAA_dry1 + lastInputADAA_dry2) / 3.0;
    lastInputADAA_dry2 = lastInputADAA_dry1;
    lastInputADAA_dry1 = hpfOut;

    double apfOut = apfCoef * out + apfStateIn_dry - apfCoef * apfStateOut_dry;
    apfStateIn_dry = out;
    apfStateOut_dry = apfOut;

    return static_cast<float>(apfOut);
}

float Preamp_Tube::processSample(float input, float driveParam, float charParam, float asymParam, float ageParam)
{
    if (driveParam != lastDriveParam) {
        double normalizedDrive = static_cast<double>(driveParam) / 100.0;
        double driveDb = std::pow(normalizedDrive, 3.0) * 30.0;
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
        double fc_hpf = 30.0;
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
        sagRatio = juce::jmap(static_cast<double>(ageParam), 0.0, 100.0, 0.0, 0.4);
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

    if (isAnalyzerMode || driveParam == 0.0f) {
        softclipOut = x;
        lastInputADAA2 = lastInputADAA1;
        lastInputADAA1 = x;
    }
    else {
        double diff1 = x - lastInputADAA1;
        double diff2 = lastInputADAA1 - lastInputADAA2;
        double diff_total = x - lastInputADAA2;

        if (std::abs(diff_total) <= ADAA_EPS) {
            softclipOut = ADAA_Math::fx_tube((x + lastInputADAA2) * 0.5);
        }
        else if (std::abs(diff1) <= ADAA_EPS) {
            double div2 = (ADAA_Math::F2_tube(lastInputADAA1) - ADAA_Math::F2_tube(lastInputADAA2)) / diff2;
            softclipOut = (2.0 / diff_total) * (ADAA_Math::F1_tube((x + lastInputADAA1) * 0.5) - div2);
        }
        else if (std::abs(diff2) <= ADAA_EPS) {
            double div1 = (ADAA_Math::F2_tube(x) - ADAA_Math::F2_tube(lastInputADAA1)) / diff1;
            softclipOut = (2.0 / diff_total) * (div1 - ADAA_Math::F1_tube((lastInputADAA1 + lastInputADAA2) * 0.5));
        }
        else {
            double div1 = (ADAA_Math::F2_tube(x) - ADAA_Math::F2_tube(lastInputADAA1)) / diff1;
            double div2 = (ADAA_Math::F2_tube(lastInputADAA1) - ADAA_Math::F2_tube(lastInputADAA2)) / diff2;
            softclipOut = (2.0 / diff_total) * (div1 - div2);
        }

        // ★ ADAA2 ノッチ補償フィルタ
        double compensatedOut = softclipOut - 0.12 * lastInputADAA_dry2;
        softclipOut = compensatedOut;

        lastInputADAA2 = lastInputADAA1;
        lastInputADAA1 = x;
    }

    double outputWithoutDC = softclipOut - fxBias;

    double apfIn = outputWithoutDC;
    double apfOut = apfCoef * apfIn + apfStateIn - apfCoef * apfStateOut;
    apfStateIn = apfIn;
    apfStateOut = apfOut;

    double diffOut = (apfOut - alpha * lastSoftclipOut) / oneMinusAlpha;
    lastSoftclipOut = apfOut;

    return static_cast<float>(diffOut * makeUp);
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

float Preamp_SSL::processDrySample(float input, float driveParam, float charParam, float asymParam, float ageParam) {
    double dInput = static_cast<double>(input);
    double out = (dInput + lastInputADAA_dry1 + lastInputADAA_dry2) / 3.0;
    lastInputADAA_dry2 = lastInputADAA_dry1;
    lastInputADAA_dry1 = dInput;
    return static_cast<float>(out);
}

float Preamp_SSL::processSample(float input, float driveParam, float charParam, float asymParam, float ageParam)
{
    if (driveParam != lastDriveParam) {
        double normalizedDrive = static_cast<double>(driveParam) / 100.0;
        double driveDb = std::pow(normalizedDrive, 3.0) * 15.0;
        gain = juce::Decibels::decibelsToGain(driveDb);
        makeUp = 1.0 / (1.0 + (gain - 1.0) * 0.2);
        lastDriveParam = driveParam;
    }

    if (charParam != lastCharParam) {
        double charNormalized = static_cast<double>(charParam) / 100.0;
        mixEven = charNormalized * 0.3;
        mixOdd = 1.0 - mixEven;
        lastCharParam = charParam;
    }

    if (asymParam != lastAsymParam) {
        bias = juce::jmap(static_cast<double>(asymParam), 0.0, 100.0, 0.0, 0.2);
        fxBias = (ADAA_Math::fx_chebyshev_even(bias) * mixEven) + (ADAA_Math::fx_hardknee(bias) * mixOdd);
        lastAsymParam = asymParam;
    }

    double drivenSignal = static_cast<double>(input) * gain;
    double x = drivenSignal + bias;
    double softclipOut = 0.0;

    if (isAnalyzerMode || driveParam == 0.0f) {
        softclipOut = x;
        lastInputADAA2 = lastInputADAA1;
        lastInputADAA1 = x;
    }
    else {
        double diff1 = x - lastInputADAA1;
        double diff2 = lastInputADAA1 - lastInputADAA2;
        double diff_total = x - lastInputADAA2;

        auto calcFx = [&](double val) -> double {
            return (ADAA_Math::fx_chebyshev_even(val) * mixEven) + (ADAA_Math::fx_hardknee(val) * mixOdd);
            };
        auto calcF1 = [&](double val) -> double {
            return (ADAA_Math::F1_chebyshev_even(val) * mixEven) + (ADAA_Math::F1_hardknee(val) * mixOdd);
            };
        auto calcF2 = [&](double val) -> double {
            return (ADAA_Math::F2_chebyshev_even(val) * mixEven) + (ADAA_Math::F2_hardknee(val) * mixOdd);
            };

        if (std::abs(diff_total) <= ADAA_EPS) {
            softclipOut = calcFx((x + lastInputADAA2) * 0.5);
        }
        else if (std::abs(diff1) <= ADAA_EPS) {
            double div2 = (calcF2(lastInputADAA1) - calcF2(lastInputADAA2)) / diff2;
            softclipOut = (2.0 / diff_total) * (calcF1((x + lastInputADAA1) * 0.5) - div2);
        }
        else if (std::abs(diff2) <= ADAA_EPS) {
            double div1 = (calcF2(x) - calcF2(lastInputADAA1)) / diff1;
            softclipOut = (2.0 / diff_total) * (div1 - calcF1((lastInputADAA1 + lastInputADAA2) * 0.5));
        }
        else {
            double div1 = (calcF2(x) - calcF2(lastInputADAA1)) / diff1;
            double div2 = (calcF2(lastInputADAA1) - calcF2(lastInputADAA2)) / diff2;
            softclipOut = (2.0 / diff_total) * (div1 - div2);
        }

        // ★ ADAA2 ノッチ補償フィルタ
        double compensatedOut = softclipOut - 0.12 * lastInputADAA_dry2;
        softclipOut = compensatedOut;

        lastInputADAA2 = lastInputADAA1;
        lastInputADAA1 = x;
    }

    double outputWithoutDC = softclipOut - fxBias;
    return static_cast<float>(outputWithoutDC * makeUp);
}