#pragma once

#include <JuceHeader.h>

class IInputTransformerEngine {
public:
    virtual ~IInputTransformerEngine() = default;
    virtual void prepare(double sampleRate) = 0;
    virtual float processSample(float input) = 0;
    // ★ 追加: 高サンプルレート基準でのADAA遅延サンプル数
    virtual float getLatencySamples() const { return 0.0f; }
};

class IPreampEngine {
public:
    virtual ~IPreampEngine() = default;
    virtual void prepare(double sampleRate) = 0;
    virtual float processSample(float input, float drive, float character, float asymmetry, float age) = 0;
    // ★ 追加
    virtual float getLatencySamples() const { return 0.0f; }
};

class IOutputTransformerEngine {
public:
    virtual ~IOutputTransformerEngine() = default;
    virtual void prepare(double sampleRate) = 0;

    // Wet用の通常の非線形処理
    virtual float processSample(float input, float colorParam, float airParam, float ageParam) = 0;

    // ★ 追加: Dry用のGhost Path処理 (非線形歪みをバイパスし、IIRフィルタの位相回転とEQカーブのみを適用する)
    virtual float processDrySample(float input, float airParam, float ageParam) = 0;

    // ★ 追加
    virtual float getLatencySamples() const { return 0.0f; }
};