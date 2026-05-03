#pragma once
#include <JuceHeader.h>
#include "../Core/EngineInterfaces.h"
#include "../Core/ADAA_Math.h"

//==============================================================================
// INPUT TRANSFORMERS (微小信号・1引数)
//==============================================================================
class InputTransformer_None : public IInputTransformerEngine {
public:
    void prepare(double) override {}
    float processSample(float input) override { return input; }
};

class InputTransformer_Nickel : public IInputTransformerEngine {
public:
    void prepare(double sampleRate) override;
    float processSample(float input) override;
private:
    double fs = 44100.0;
    double lastInputADAA = 0.0;
};

// 以降はプロトタイプ空箱
class InputTransformer_Steel : public IInputTransformerEngine {
public: void prepare(double) override {} float processSample(float input) override { return input; }
};
class InputTransformer_Iron : public IInputTransformerEngine {
public: void prepare(double) override {} float processSample(float input) override { return input; }
};
class InputTransformer_Amorphous : public IInputTransformerEngine {
public: void prepare(double) override {} float processSample(float input) override { return input; }
};


//==============================================================================
// OUTPUT TRANSFORMERS (大信号・4引数)
//==============================================================================
class OutputTransformer_None : public IOutputTransformerEngine {
public:
    void prepare(double) override {}
    float processSample(float input, float, float, float) override { return input; }
};

class OutputTransformer_Steel : public IOutputTransformerEngine {
public:
    void prepare(double sampleRate) override;
    float processSample(float input, float colorParam, float airParam, float ageParam) override;
private:
    double fs = 44100.0;

    // SteelTransformerから完全移植したフィルタ状態変数
    double hpfState = 0.0;
    double lastInput = 0.0;
    double apfState = 0.0;
    double lastApfInput = 0.0;
    double lpfState = 0.0;

    // Biquad (Air EQ) 状態変数
    double x1 = 0.0, x2 = 0.0, y1 = 0.0, y2 = 0.0;
    double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;

    // キャッシュ変数
    float lastColorParam = -1.0f;
    float lastAirParam = -1.0f;
    float lastAgeParam = -1.0f;

    double alphaHpf = 0.0;
    double apfAlpha = 0.0;
    double alphaLpf = 0.0;
};

// 以降はプロトタイプ空箱
class OutputTransformer_Nickel : public IOutputTransformerEngine {
public: void prepare(double) override {} float processSample(float input, float, float, float) override { return input; }
};
class OutputTransformer_Iron : public IOutputTransformerEngine {
public: void prepare(double) override {} float processSample(float input, float, float, float) override { return input; }
};
class OutputTransformer_Amorphous : public IOutputTransformerEngine {
public: void prepare(double) override {} float processSample(float input, float, float, float) override { return input; }
};