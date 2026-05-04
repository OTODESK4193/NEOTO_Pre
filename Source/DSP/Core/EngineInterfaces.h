#pragma once
#include <JuceHeader.h>

class IInputTransformerEngine {
public:
    virtual ~IInputTransformerEngine() = default;
    virtual void prepare(double sampleRate) = 0;
    virtual float processSample(float input) = 0;

    // ★ True Ghost Path: 非線形をバイパスし、ADAAの線形遅延とフィルタのみを適用
    virtual float processDrySample(float input) = 0;
};

class IPreampEngine {
public:
    virtual ~IPreampEngine() = default;
    virtual void prepare(double sampleRate) = 0;
    virtual float processSample(float input, float drive, float character, float asymmetry, float age) = 0;

    // ★ True Ghost Path
    virtual float processDrySample(float input, float drive, float character, float asymmetry, float age) = 0;
};

class IOutputTransformerEngine {
public:
    virtual ~IOutputTransformerEngine() = default;
    virtual void prepare(double sampleRate) = 0;
    virtual float processSample(float input, float colorParam, float airParam, float ageParam) = 0;

    // ★ True Ghost Path
    virtual float processDrySample(float input, float airParam, float ageParam) = 0;
};