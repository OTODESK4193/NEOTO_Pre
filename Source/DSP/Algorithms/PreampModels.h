#pragma once
#include <JuceHeader.h>
#include "../Core/EngineInterfaces.h"
#include "../Core/ADAA_Math.h"

//==============================================================================
// 1. API Style
//==============================================================================
class Preamp_API : public IPreampEngine {
public:
    Preamp_API() = default;
    void prepare(double sampleRate) override;
    float processSample(float input, float driveParam, float charParam, float asymParam, float ageParam) override;
    float processDrySample(float input, float driveParam, float charParam, float asymParam, float ageParam) override;
    void setAnalyzerMode(bool isAnalyzer) override { isAnalyzerMode = isAnalyzer; }

private:
    double fs = 44100.0;
    bool isAnalyzerMode = false;
    double integratorState = 0.0;
    double lastInputADAA1 = 0.0;
    double lastInputADAA2 = 0.0;
    double lastInputADAA_dry1 = 0.0;
    double lastInputADAA_dry2 = 0.0;

    // API APF
    double apfStateIn = 0.0;
    double apfStateOut = 0.0;
    double apfCoef = 0.0;
    double apfStateIn_dry = 0.0;
    double apfStateOut_dry = 0.0;

    // ★ 追加: API アナログ特性のフィルタ (10Hz HPF / 22kHz LPF) 用ステート
    double alphaHpf = 0.0;
    double alphaLpf = 0.0;
    double hpfState = 0.0;
    double lpfState = 0.0;
    double lastInput = 0.0;
    double hpfState_dry = 0.0;
    double lpfState_dry = 0.0;
    double lastInput_dry = 0.0;

    double lastSoftclipOut = 0.0;
    double envState = 0.0;

    float lastDriveParam = -1.0f;
    float lastCharParam = -1.0f;
    float lastAsymParam = -1.0f;
    float lastAgeParam = -1.0f;

    double gain = 1.0, makeUp = 1.0, alpha = 0.0, oneMinusAlpha = 1.0;
    double bias = 0.0, fxBias = 0.0, envAttackCoef = 0.0, envReleaseCoef = 0.0, sagRatio = 0.0;
    double mixEven = 0.5, mixOdd = 0.5;
};

//==============================================================================
// 2. Neve Style (1073)
//==============================================================================
class Preamp_Neve : public IPreampEngine {
public:
    Preamp_Neve() = default;
    void prepare(double sampleRate) override;
    float processSample(float input, float driveParam, float charParam, float asymParam, float ageParam) override;
    float processDrySample(float input, float driveParam, float charParam, float asymParam, float ageParam) override;
    void setAnalyzerMode(bool isAnalyzer) override { isAnalyzerMode = isAnalyzer; }

private:
    double fs = 44100.0;
    bool isAnalyzerMode = false;
    double integratorState = 0.0;
    double lastInputADAA1 = 0.0;
    double lastInputADAA2 = 0.0;
    double lastInputADAA_dry1 = 0.0;
    double lastInputADAA_dry2 = 0.0;
    double apfStateIn = 0.0;
    double apfStateOut = 0.0;
    double apfCoef = 0.0;
    double apfStateIn_dry = 0.0;
    double apfStateOut_dry = 0.0;
    double lastSoftclipOut = 0.0;
    double envState = 0.0;

    float lastDriveParam = -1.0f, lastCharParam = -1.0f, lastAsymParam = -1.0f, lastAgeParam = -1.0f;
    double gain = 1.0, makeUp = 1.0, alpha = 0.0, oneMinusAlpha = 1.0;
    double bias = 0.0, fxBias = 0.0, envAttackCoef = 0.0, envReleaseCoef = 0.0, sagRatio = 0.0;
    double mixEven = 0.8, mixOdd = 0.2; // Neveは偶数次主体
};

//==============================================================================
// 3. Vintage Tube Style (V76s)
//==============================================================================
class Preamp_Tube : public IPreampEngine {
public:
    Preamp_Tube() = default;
    void prepare(double sampleRate) override;
    float processSample(float input, float driveParam, float charParam, float asymParam, float ageParam) override;
    float processDrySample(float input, float driveParam, float charParam, float asymParam, float ageParam) override;
    void setAnalyzerMode(bool isAnalyzer) override { isAnalyzerMode = isAnalyzer; }

private:
    double fs = 44100.0;
    bool isAnalyzerMode = false;
    double hpfState = 0.0; // V76s特有の低域位相反転用
    double lastInput = 0.0;
    double alphaHpf = 0.0;
    double integratorState = 0.0;
    double lastInputADAA1 = 0.0;
    double lastInputADAA2 = 0.0;
    double lastInputADAA_dry1 = 0.0;
    double lastInputADAA_dry2 = 0.0;
    double apfStateIn = 0.0;
    double apfStateOut = 0.0;
    double apfCoef = 0.0;
    double apfStateIn_dry = 0.0;
    double apfStateOut_dry = 0.0;
    double hpfState_dry = 0.0;
    double lastInput_dry = 0.0;
    double lastSoftclipOut = 0.0;
    double envState = 0.0;

    float lastDriveParam = -1.0f, lastCharParam = -1.0f, lastAsymParam = -1.0f, lastAgeParam = -1.0f;
    double gain = 1.0, makeUp = 1.0, alpha = 0.0, oneMinusAlpha = 1.0;
    double bias = 0.0, fxBias = 0.0, envAttackCoef = 0.0, envReleaseCoef = 0.0, sagRatio = 0.0;
    double mixEven = 0.5, mixOdd = 0.5;
};

//==============================================================================
// 4. SSL Modern Style
//==============================================================================
class Preamp_SSL : public IPreampEngine {
public:
    Preamp_SSL() = default;
    void prepare(double sampleRate) override;
    float processSample(float input, float driveParam, float charParam, float asymParam, float ageParam) override;
    float processDrySample(float input, float driveParam, float charParam, float asymParam, float ageParam) override;
    void setAnalyzerMode(bool isAnalyzer) override { isAnalyzerMode = isAnalyzer; }

private:
    double fs = 44100.0;
    bool isAnalyzerMode = false;
    double lastInputADAA1 = 0.0;
    double lastInputADAA2 = 0.0;
    double lastInputADAA_dry1 = 0.0;
    double lastInputADAA_dry2 = 0.0;
    float lastDriveParam = -1.0f, lastCharParam = -1.0f, lastAsymParam = -1.0f, lastAgeParam = -1.0f;
    double gain = 1.0, makeUp = 1.0;
    double mixEven = 0.0, mixOdd = 1.0; // SSLは奇数次主体、コンプ感なし
    double bias = 0.0, fxBias = 0.0;
};

//==============================================================================
// 5 & 6. Dummy Models (Modern 1 & Modern 2)
//==============================================================================
class Preamp_Modern1 : public IPreampEngine {
public:
    void prepare(double) override {}
    float processSample(float input, float, float, float, float) override { return input; }
    float processDrySample(float input, float, float, float, float) override { return input; }
    void setAnalyzerMode(bool) override {}
};

class Preamp_Modern2 : public IPreampEngine {
public:
    void prepare(double) override {}
    float processSample(float input, float, float, float, float) override { return input; }
    float processDrySample(float input, float, float, float, float) override { return input; }
    void setAnalyzerMode(bool) override {}
};