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
    float processSample(float input, float color, float air, float age) override;
private:
    double fs = 44100.0;
    // TODO: 後ほど既存のSteelTransformerのメンバ変数をここにコピペします
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