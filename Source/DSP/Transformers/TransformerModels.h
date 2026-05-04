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

class InputTransformer_Steel : public IInputTransformerEngine {
public:
    void prepare(double sampleRate) override;
    float processSample(float input) override;
private:
    double fs = 44100.0;
    double lastInputADAA = 0.0;
    double hpfState = 0.0;
    double lastInput = 0.0;
    double alphaHpf = 0.0;
};

class InputTransformer_Iron : public IInputTransformerEngine {
public:
    void prepare(double sampleRate) override;
    float processSample(float input) override;
private:
    double fs = 44100.0;
    double lastInputADAA = 0.0;
};

class InputTransformer_Amorphous : public IInputTransformerEngine {
public:
    void prepare(double sampleRate) override;
    float processSample(float input) override;
private:
    double fs = 44100.0;
    double lastInputADAA = 0.0;
};

// ==============================================================================
// ★ Tellinen ヒステリシス・プロセッサ (出力トランス用コアエンジン)
// ==============================================================================
class TellinenHysteresis {
public:
    TellinenHysteresis() = default;

    void prepare() {
        last_x = 0.0; last_y = 0.0;
        last_Yl = 0.0; last_Yu = 0.0;
    }

    inline float processSample(float input, double drive, double Hc) {
        double x = static_cast<double>(input);
        if (std::abs(x - last_x) < 1e-8) return static_cast<float>(last_y);

        auto f = [drive](double v) { return std::tanh(drive * v); };

        double Yu = f(x + Hc);
        double Yl = f(x - Hc);
        double y_next = last_y;

        const double eps = 1e-8;
        double range = Yu - Yl + eps;

        if (x > last_x) {
            double dYl = Yl - last_Yl;
            double scale = (Yu - last_y) / range;
            y_next = last_y + dYl * std::clamp(scale, 0.0, 1.0);
        }
        else {
            double dYu = Yu - last_Yu;
            double scale = (last_y - Yl) / range;
            y_next = last_y + dYu * std::clamp(scale, 0.0, 1.0);
        }

        last_x = x; last_y = y_next;
        last_Yl = Yl; last_Yu = Yu;

        return static_cast<float>(y_next);
    }

private:
    double last_x = 0.0, last_y = 0.0, last_Yl = 0.0, last_Yu = 0.0;
};

//==============================================================================
// OUTPUT TRANSFORMERS (大信号・4引数)
//==============================================================================

class OutputTransformer_None : public IOutputTransformerEngine {
public:
    void prepare(double) override {}
    float processSample(float input, float, float, float) override { return input; }
};

class OutputTransformer_Nickel : public IOutputTransformerEngine {
public:
    void prepare(double) override {}
    float processSample(float input, float, float, float) override { return input; }
};

class OutputTransformer_Amorphous : public IOutputTransformerEngine {
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

    // フィルタ状態変数
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

    // ヒステリシス関連のパラメータとエンジン
    double hystDrive = 1.0, hystHc = 0.1;
    TellinenHysteresis hysteresisEngine;
};

class OutputTransformer_Iron : public IOutputTransformerEngine {
public:
    void prepare(double sampleRate) override;
    float processSample(float input, float colorParam, float airParam, float ageParam) override;
private:
    double fs = 44100.0;

    // フィルタ状態変数 (IronはAPFなし)
    double hpfState = 0.0;
    double lastInput = 0.0;
    double lpfState = 0.0;

    // Biquad (Air EQ) 状態変数
    double x1 = 0.0, x2 = 0.0, y1 = 0.0, y2 = 0.0;
    double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;

    // キャッシュ変数
    float lastColorParam = -1.0f;
    float lastAirParam = -1.0f;
    float lastAgeParam = -1.0f;

    double alphaHpf = 0.0;
    double alphaLpf = 0.0;

    // ヒステリシス関連のパラメータとエンジン
    double hystDrive = 1.0, hystHc = 0.05;
    TellinenHysteresis hysteresisEngine;
};