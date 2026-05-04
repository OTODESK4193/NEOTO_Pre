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

private:
    double fs = 44100.0;
    double integratorState = 0.0;
    double lastInputADAA = 0.0;
    double lastInputADAA_dry = 0.0; // ★ Dry用のADAA状態変数
    double lastSoftclipOut = 0.0;
    double envState = 0.0;

    float lastDriveParam = -1.0f;
    float lastCharParam = -1.0f;
    float lastAsymParam = -1.0f;
    float lastAgeParam = -1.0f;

    double gain = 1.0, makeUp = 1.0, alpha = 0.0, oneMinusAlpha = 1.0;
    double bias = 0.0, fxBias = 0.0, envAttackCoef = 0.0, envReleaseCoef = 0.0, sagRatio = 0.0;

    double mixEven = 0.5;
    double mixOdd = 0.5;
};

//==============================================================================
// 2〜6. Dummy Models
//==============================================================================
class Preamp_Neve : public IPreampEngine {
public:
    void prepare(double) override {}
    float processSample(float input, float, float, float, float) override { return input; }
    float processDrySample(float input, float, float, float, float) override { return input; }
};

class Preamp_Tube : public IPreampEngine {
public:
    void prepare(double) override {}
    float processSample(float input, float, float, float, float) override { return input; }
    float processDrySample(float input, float, float, float, float) override { return input; }
};

class Preamp_SSL : public IPreampEngine {
public:
    void prepare(double) override {}
    float processSample(float input, float, float, float, float) override { return input; }
    float processDrySample(float input, float, float, float, float) override { return input; }
};

class Preamp_Modern1 : public IPreampEngine {
public:
    void prepare(double) override {}
    float processSample(float input, float, float, float, float) override { return input; }
    float processDrySample(float input, float, float, float, float) override { return input; }
};

class Preamp_Modern2 : public IPreampEngine {
public:
    void prepare(double) override {}
    float processSample(float input, float, float, float, float) override { return input; }
    float processDrySample(float input, float, float, float, float) override { return input; }
};