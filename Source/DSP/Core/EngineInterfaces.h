#pragma once

class IInputTransformerEngine {
public:
    virtual ~IInputTransformerEngine() = default;
    virtual void prepare(double sampleRate) = 0;
    virtual float processSample(float input) = 0;
    virtual float processDrySample(float input) = 0;
};

class IPreampEngine {
public:
    virtual ~IPreampEngine() = default;
    virtual void prepare(double sampleRate) = 0;
    // 互換性維持のため ageParam を5番目に戻し、colorParam をデフォルト引数として末尾に配置
    virtual float processSample(float input, float driveParam, float charParam, float asymParam, float ageParam, float colorParam = 50.0f) = 0;
    virtual float processDrySample(float input, float driveParam, float charParam, float asymParam, float ageParam, float colorParam = 50.0f) = 0;
    virtual void setAnalyzerMode(bool isAnalyzer) = 0;
};

class IOutputTransformerEngine {
public:
    virtual ~IOutputTransformerEngine() = default;
    virtual void prepare(double sampleRate) = 0;
    virtual float processSample(float input, float colorParam, float airParam, float ageParam) = 0;
    virtual float processDrySample(float input, float airParam, float ageParam) = 0;
    virtual void setAnalyzerMode(bool isAnalyzer) = 0;
};