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
    float processSample(float input, float driveParam, float charParam, float asymParam, float ageParam, float colorParam = 50.0f) override;
    float processDrySample(float input, float driveParam, float charParam, float asymParam, float ageParam, float colorParam = 50.0f) override;
    void setAnalyzerMode(bool isAnalyzer) override { isAnalyzerMode = isAnalyzer; }
private:
    double fs = 44100.0; bool isAnalyzerMode = false;
    double integratorState = 0.0;
    double lastInputADAA1 = 0.0, lastInputADAA2 = 0.0;
    double lastInputADAA_dry1 = 0.0, lastInputADAA_dry2 = 0.0;
    double apfStateIn = 0.0, apfStateOut = 0.0, apfCoef = 0.0;
    double apfStateIn_dry = 0.0, apfStateOut_dry = 0.0;
    double alphaHpf = 0.0, alphaLpf = 0.0;
    double hpfState = 0.0, lpfState = 0.0, lastInput = 0.0;
    double hpfState_dry = 0.0, lpfState_dry = 0.0, lastInput_dry = 0.0;
    double lastSoftclipOut = 0.0, envState = 0.0;
    float lastDriveParam = -1.0f, lastCharParam = -1.0f, lastAsymParam = -1.0f, lastAgeParam = -1.0f, lastColorParam = -1.0f;
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
    float processSample(float input, float driveParam, float charParam, float asymParam, float ageParam, float colorParam = 50.0f) override;
    float processDrySample(float input, float driveParam, float charParam, float asymParam, float ageParam, float colorParam = 50.0f) override;
    void setAnalyzerMode(bool isAnalyzer) override { isAnalyzerMode = isAnalyzer; }
private:
    double fs = 44100.0; bool isAnalyzerMode = false;
    double integratorState = 0.0;
    double lastInputADAA1 = 0.0, lastInputADAA2 = 0.0;
    double lastInputADAA_dry1 = 0.0, lastInputADAA_dry2 = 0.0;
    double apfStateIn = 0.0, apfStateOut = 0.0, apfCoef = 0.0;
    double apfStateIn_dry = 0.0, apfStateOut_dry = 0.0;
    double lastSoftclipOut = 0.0, envState = 0.0;
    float lastDriveParam = -1.0f, lastCharParam = -1.0f, lastAsymParam = -1.0f, lastAgeParam = -1.0f, lastColorParam = -1.0f;
    double gain = 1.0, makeUp = 1.0, alpha = 0.0, oneMinusAlpha = 1.0;
    double bias = 0.0, fxBias = 0.0, envAttackCoef = 0.0, envReleaseCoef = 0.0, sagRatio = 0.0;
    double mixEven = 0.8, mixOdd = 0.2;
};

//==============================================================================
// 3. Vintage Tube Style (V76s)
//==============================================================================
class Preamp_Tube : public IPreampEngine {
public:
    Preamp_Tube() = default;
    void prepare(double sampleRate) override;
    float processSample(float input, float driveParam, float charParam, float asymParam, float ageParam, float colorParam = 50.0f) override;
    float processDrySample(float input, float driveParam, float charParam, float asymParam, float ageParam, float colorParam = 50.0f) override;
    void setAnalyzerMode(bool isAnalyzer) override { isAnalyzerMode = isAnalyzer; }
private:
    double fs = 44100.0; bool isAnalyzerMode = false;
    double hpfState = 0.0, lastInput = 0.0, alphaHpf = 0.0;
    double integratorState = 0.0;
    double lastInputADAA1 = 0.0, lastInputADAA2 = 0.0;
    double lastInputADAA_dry1 = 0.0, lastInputADAA_dry2 = 0.0;
    double apfStateIn = 0.0, apfStateOut = 0.0, apfCoef = 0.0;
    double apfStateIn_dry = 0.0, apfStateOut_dry = 0.0;
    double hpfState_dry = 0.0, lastInput_dry = 0.0;
    double lastSoftclipOut = 0.0, envState = 0.0;
    float lastDriveParam = -1.0f, lastCharParam = -1.0f, lastAsymParam = -1.0f, lastAgeParam = -1.0f, lastColorParam = -1.0f;
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
    float processSample(float input, float driveParam, float charParam, float asymParam, float ageParam, float colorParam = 50.0f) override;
    float processDrySample(float input, float driveParam, float charParam, float asymParam, float ageParam, float colorParam = 50.0f) override;
    void setAnalyzerMode(bool isAnalyzer) override { isAnalyzerMode = isAnalyzer; }
private:
    double fs = 44100.0; bool isAnalyzerMode = false;
    double lastInputADAA1 = 0.0, lastInputADAA2 = 0.0;
    double lastInputADAA_dry1 = 0.0, lastInputADAA_dry2 = 0.0;
    double lastSoftclipOut = 0.0; // ★ 欠落していた変数を追加
    float lastDriveParam = -1.0f, lastCharParam = -1.0f, lastAsymParam = -1.0f, lastAgeParam = -1.0f, lastColorParam = -1.0f;
    double gain = 1.0, makeUp = 1.0;
    double mixEven = 0.0, mixOdd = 1.0;
    double bias = 0.0, fxBias = 0.0;
};

//==============================================================================
// 5. TG2 Style (Modern 1 - ハイパンプ＆ディスクリート非対称)
//==============================================================================
class Preamp_Modern1 : public IPreampEngine {
public:
    Preamp_Modern1() = default;
    void prepare(double sampleRate) override;
    float processSample(float input, float driveParam, float charParam, float asymParam, float ageParam, float colorParam = 50.0f) override;
    float processDrySample(float input, float driveParam, float charParam, float asymParam, float ageParam, float colorParam = 50.0f) override;
    void setAnalyzerMode(bool isAnalyzer) override { isAnalyzerMode = isAnalyzer; }
private:
    double fs = 44100.0; bool isAnalyzerMode = false;
    double lastInputADAA1 = 0.0, lastInputADAA2 = 0.0, lastInputADAA_dry1 = 0.0, lastInputADAA_dry2 = 0.0;
    double impLpfState = 0.0, impHpfState = 0.0, lastInput = 0.0;
    double impLpfState_dry = 0.0, impHpfState_dry = 0.0, lastInput_dry = 0.0;
    double alphaImpedanceLpf = 0.0, alphaImpedanceHpf = 0.0;
    double color_x1 = 0.0, color_x2 = 0.0, color_y1 = 0.0, color_y2 = 0.0;
    double color_b0 = 1.0, color_b1 = 0.0, color_b2 = 0.0, color_a1 = 0.0, color_a2 = 0.0;
    double color_x1_dry = 0.0, color_x2_dry = 0.0, color_y1_dry = 0.0, color_y2_dry = 0.0;
    float lastDriveParam = -1.0f, lastCharParam = -1.0f, lastAsymParam = -1.0f, lastAgeParam = -1.0f, lastColorParam = -1.0f;
    double gain = 1.0, makeUp = 1.0, mixEven = 0.5, mixOdd = 0.5, bias = 0.0, fxBias = 0.0;
};

//==============================================================================
// 6. B173 Style (Modern 2 - ブライトNeve系＆太さ)
//==============================================================================
class Preamp_Modern2 : public IPreampEngine {
public:
    Preamp_Modern2() = default;
    void prepare(double sampleRate) override;
    float processSample(float input, float driveParam, float charParam, float asymParam, float ageParam, float colorParam = 50.0f) override;
    float processDrySample(float input, float driveParam, float charParam, float asymParam, float ageParam, float colorParam = 50.0f) override;
    void setAnalyzerMode(bool isAnalyzer) override { isAnalyzerMode = isAnalyzer; }
private:
    double fs = 44100.0; bool isAnalyzerMode = false;
    double lastInputADAA1 = 0.0, lastInputADAA2 = 0.0, lastInputADAA_dry1 = 0.0, lastInputADAA_dry2 = 0.0;
    double color_x1 = 0.0, color_x2 = 0.0, color_y1 = 0.0, color_y2 = 0.0;
    double color_b0 = 1.0, color_b1 = 0.0, color_b2 = 0.0, color_a1 = 0.0, color_a2 = 0.0;
    double color_x1_dry = 0.0, color_x2_dry = 0.0, color_y1_dry = 0.0, color_y2_dry = 0.0;
    double lpfState = 0.0, alphaAgeLpf = 0.0, lpfState_dry = 0.0;
    float lastDriveParam = -1.0f, lastCharParam = -1.0f, lastAsymParam = -1.0f, lastAgeParam = -1.0f, lastColorParam = -1.0f;
    double gain = 1.0, makeUp = 1.0, mixEven = 0.8, mixOdd = 0.2, bias = 0.0, fxBias = 0.0;
};