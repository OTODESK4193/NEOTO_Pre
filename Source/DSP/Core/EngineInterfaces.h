#pragma once

// プリアンプの共通設計図
class IPreampEngine {
public:
    virtual ~IPreampEngine() = default;
    virtual void prepare(double sampleRate) = 0;
    virtual float processSample(float input, float drive, float character, float asymmetry, float age) = 0;
};

// 入力トランスの共通設計図 (微小信号・サチュレーション用)
class IInputTransformerEngine {
public:
    virtual ~IInputTransformerEngine() = default;
    virtual void prepare(double sampleRate) = 0;
    virtual float processSample(float input) = 0;
};

// 出力トランスの共通設計図 (大信号・ヒステリシス・カラー用)
class IOutputTransformerEngine {
public:
    virtual ~IOutputTransformerEngine() = default;
    virtual void prepare(double sampleRate) = 0;
    virtual float processSample(float input, float color, float air, float age) = 0;
};