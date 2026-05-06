#pragma once
#include <JuceHeader.h>
#include "../Core/EngineInterfaces.h"
#include "../Core/ADAA_Math.h"

// ==============================================================================
// INPUT TRANSFORMERS
// ==============================================================================
class InputTransformer_None : public IInputTransformerEngine {
public:
    void prepare(double) override {}
    float processSample(float input) override { return input; }
    float processDrySample(float input) override { return input; }
};

class InputTransformer_Nickel : public IInputTransformerEngine {
public:
    void prepare(double sampleRate) override;
    float processSample(float input) override;
    float processDrySample(float input) override;
private:
    double fs = 44100.0;
    double lastInputADAA = 0.0, lastInputADAA_dry = 0.0;
};

class InputTransformer_Steel : public IInputTransformerEngine {
public:
    void prepare(double sampleRate) override;
    float processSample(float input) override;
    float processDrySample(float input) override;
private:
    double fs = 44100.0;
    double lastInputADAA = 0.0, hpfState = 0.0, lastInput = 0.0, alphaHpf = 0.0;
    double lastInputADAA_dry = 0.0, hpfState_dry = 0.0, lastInput_dry = 0.0;
};

class InputTransformer_Iron : public IInputTransformerEngine {
public:
    void prepare(double sampleRate) override;
    float processSample(float input) override;
    float processDrySample(float input) override;
private:
    double fs = 44100.0;
    double lastInputADAA = 0.0, lastInputADAA_dry = 0.0;
};

class InputTransformer_Amorphous : public IInputTransformerEngine {
public:
    void prepare(double sampleRate) override;
    float processSample(float input) override;
    float processDrySample(float input) override;
private:
    double fs = 44100.0;
    double lastInputADAA = 0.0, lastInputADAA_dry = 0.0;
};

class InputTransformer_Carnhill : public IInputTransformerEngine {
public:
    void prepare(double sampleRate) override;
    float processSample(float input) override;
    float processDrySample(float input) override;
private:
    double fs = 44100.0;
    double lastInputADAA = 0.0, lastInputADAA_dry = 0.0;
};

class InputTransformer_Cinemag : public IInputTransformerEngine {
public:
    void prepare(double sampleRate) override;
    float processSample(float input) override;
    float processDrySample(float input) override;
private:
    double fs = 44100.0;
    double lastInputADAA = 0.0, lastInputADAA_dry = 0.0;
};

// ==============================================================================
// Hysteresis Processors
// ==============================================================================
class TellinenHysteresis {
public:
    TellinenHysteresis() = default;
    void prepare() { last_x = 0.0; last_y = 0.0; last_Yl = 0.0; last_Yu = 0.0; }
    inline float processSample(float input, double drive, double Hc) {
        double x = static_cast<double>(input);
        if (std::abs(x - last_x) < 1e-8) return static_cast<float>(last_y);
        auto f = [drive](double v) { return std::tanh(drive * v); };
        double Yu = f(x + Hc); double Yl = f(x - Hc); double y_next = last_y;
        const double eps = 1e-8; double range = Yu - Yl + eps;
        if (x > last_x) {
            double dYl = Yl - last_Yl; double scale = (Yu - last_y) / range;
            y_next = last_y + dYl * std::clamp(scale, 0.0, 1.0);
        }
        else {
            double dYu = Yu - last_Yu; double scale = (last_y - Yl) / range;
            y_next = last_y + dYu * std::clamp(scale, 0.0, 1.0);
        }
        last_x = x; last_y = y_next; last_Yl = Yl; last_Yu = Yu;
        return static_cast<float>(y_next);
    }
private:
    double last_x = 0.0, last_y = 0.0, last_Yl = 0.0, last_Yu = 0.0;
};

class JAHysteresis {
public:
    JAHysteresis() = default;
    void prepare() { last_H = 0.0; last_Mirr = 0.0; }
    inline double langevin(double x) {
        double ax = std::abs(x);
        if (ax < 1e-4) return x / 3.0;
        else if (ax < 0.1) return x / 3.0 - (x * x * x) / 45.0;
        else if (ax > 20.0) return (x > 0.0 ? 1.0 : -1.0) - 1.0 / x;
        else return 1.0 / std::tanh(x) - 1.0 / x;
    }
    inline float processSample(float input, double a, double k, double c) {
        double H_n = static_cast<double>(input); double dH = H_n - last_H;
        double M_an = langevin(H_n / a); double M_irr_n = last_Mirr;
        double step = std::abs(dH) / std::max(1e-5, k);
        if (dH * (M_an - last_Mirr) > 0.0) M_irr_n = (last_Mirr + M_an * step) / (1.0 + step);
        double M_n = c * M_an + (1.0 - c) * M_irr_n;
        last_H = H_n; last_Mirr = M_irr_n; return static_cast<float>(M_n);
    }
private:
    double last_H = 0.0; double last_Mirr = 0.0;
};

// ==============================================================================
// OUTPUT TRANSFORMERS
// ==============================================================================
class OutputTransformer_None : public IOutputTransformerEngine {
public:
    void prepare(double) override {}
    float processSample(float input, float, float, float) override { return input; }
    float processDrySample(float input, float, float) override { return input; }
    void setAnalyzerMode(bool) override {}
};

class OutputTransformer_Amorphous : public IOutputTransformerEngine {
public:
    void prepare(double sampleRate) override;
    float processSample(float input, float colorParam, float airParam, float ageParam) override;
    float processDrySample(float input, float airParam, float ageParam) override;
    void setAnalyzerMode(bool isAnalyzer) override { isAnalyzerMode = isAnalyzer; }
private:
    double fs = 44100.0; bool isAnalyzerMode = false;
    float lastColorParam = -1.0f, lastAirParam = -1.0f, lastAgeParam = -1.0f;
    double lpfState = 0.0, alphaLpf = 0.0;
    double lpfState_dry = 0.0;

    // Air (High Shelf @ 5kHz)
    double air_x1 = 0.0, air_x2 = 0.0, air_y1 = 0.0, air_y2 = 0.0;
    double air_x1_dry = 0.0, air_x2_dry = 0.0, air_y1_dry = 0.0, air_y2_dry = 0.0;
    double air_b0 = 1.0, air_b1 = 0.0, air_b2 = 0.0, air_a1 = 0.0, air_a2 = 0.0;
};

class OutputTransformer_Steel : public IOutputTransformerEngine {
public:
    void prepare(double sampleRate) override;
    float processSample(float input, float colorParam, float airParam, float ageParam) override;
    float processDrySample(float input, float airParam, float ageParam) override;
    void setAnalyzerMode(bool isAnalyzer) override { isAnalyzerMode = isAnalyzer; }
private:
    double fs = 44100.0; bool isAnalyzerMode = false;
    double hpfState = 0.0, lastInput = 0.0, apfState = 0.0, lastApfInput = 0.0, lpfState = 0.0;
    double hpfState_dry = 0.0, lastInput_dry = 0.0, apfState_dry = 0.0, lastApfInput_dry = 0.0, lpfState_dry = 0.0;
    float lastColorParam = -1.0f, lastAirParam = -1.0f, lastAgeParam = -1.0f;
    double alphaHpf = 0.0, apfAlpha = 0.0, alphaLpf = 0.0;
    double hystDrive = 1.0, hystHc = 0.1;
    TellinenHysteresis hysteresisEngine;
};

class OutputTransformer_Iron : public IOutputTransformerEngine {
public:
    void prepare(double sampleRate) override;
    float processSample(float input, float colorParam, float airParam, float ageParam) override;
    float processDrySample(float input, float airParam, float ageParam) override;
    void setAnalyzerMode(bool isAnalyzer) override { isAnalyzerMode = isAnalyzer; }
private:
    double fs = 44100.0; bool isAnalyzerMode = false;
    double hpfState = 0.0, lastInput = 0.0, lpfState = 0.0;
    double hpfState_dry = 0.0, lastInput_dry = 0.0, lpfState_dry = 0.0;
    float lastColorParam = -1.0f, lastAirParam = -1.0f, lastAgeParam = -1.0f;
    double alphaHpf = 0.0, alphaLpf = 0.0;
    double hystDrive = 1.0, hystHc = 0.05;
    TellinenHysteresis hysteresisEngine;
};

class OutputTransformer_Nickel : public IOutputTransformerEngine {
public:
    void prepare(double sampleRate) override;
    float processSample(float input, float colorParam, float airParam, float ageParam) override;
    float processDrySample(float input, float airParam, float ageParam) override;
    void setAnalyzerMode(bool isAnalyzer) override { isAnalyzerMode = isAnalyzer; }
private:
    double fs = 44100.0; bool isAnalyzerMode = false;
    float lastColorParam = -1.0f, lastAirParam = -1.0f, lastAgeParam = -1.0f;
    double ja_a = 4.0, ja_k = 0.0001, ja_c = 0.999;
    JAHysteresis hysteresisEngine;
    double hpfState = 0.0, lastInput = 0.0, lpfState = 0.0, alphaHpf = 0.0, alphaLpf = 0.0;
    double hpfState_dry = 0.0, lastInput_dry = 0.0, lpfState_dry = 0.0;
};

// Åö TG2 Style (Carnhill) 
class OutputTransformer_Carnhill : public IOutputTransformerEngine {
public:
    void prepare(double sampleRate) override;
    float processSample(float input, float colorParam, float airParam, float ageParam) override;
    float processDrySample(float input, float airParam, float ageParam) override;
    void setAnalyzerMode(bool isAnalyzer) override { isAnalyzerMode = isAnalyzer; }
private:
    double fs = 44100.0; bool isAnalyzerMode = false;
    float lastColorParam = -1.0f, lastAirParam = -1.0f, lastAgeParam = -1.0f;

    // Age (HPF & LPF)
    double hpfState = 0.0, lpfState = 0.0, lastInput = 0.0, alphaHpf = 0.0, alphaLpf = 0.0;
    double hpfState_dry = 0.0, lpfState_dry = 0.0, lastInput_dry = 0.0;

    // Color (High Shelf @ 3.5kHz)
    double col_x1 = 0.0, col_x2 = 0.0, col_y1 = 0.0, col_y2 = 0.0;
    double col_x1_dry = 0.0, col_x2_dry = 0.0, col_y1_dry = 0.0, col_y2_dry = 0.0;
    double col_b0 = 1.0, col_b1 = 0.0, col_b2 = 0.0, col_a1 = 0.0, col_a2 = 0.0;

    // Air (High Shelf @ 10kHz to be safe)
    double air_x1 = 0.0, air_x2 = 0.0, air_y1 = 0.0, air_y2 = 0.0;
    double air_x1_dry = 0.0, air_x2_dry = 0.0, air_y1_dry = 0.0, air_y2_dry = 0.0;
    double air_b0 = 1.0, air_b1 = 0.0, air_b2 = 0.0, air_a1 = 0.0, air_a2 = 0.0;
};

// Åö B173 Style (Cinemag)
class OutputTransformer_Cinemag : public IOutputTransformerEngine {
public:
    void prepare(double sampleRate) override;
    float processSample(float input, float colorParam, float airParam, float ageParam) override;
    float processDrySample(float input, float airParam, float ageParam) override;
    void setAnalyzerMode(bool isAnalyzer) override { isAnalyzerMode = isAnalyzer; }
private:
    double fs = 44100.0; bool isAnalyzerMode = false;
    float lastColorParam = -1.0f, lastAirParam = -1.0f, lastAgeParam = -1.0f;

    // Age (HPF & LPF)
    double hpfState = 0.0, lpfState = 0.0, lastInput = 0.0, alphaHpf = 0.0, alphaLpf = 0.0;
    double hpfState_dry = 0.0, lpfState_dry = 0.0, lastInput_dry = 0.0;

    // Color (Low Shelf @ 80Hz)
    double colL_x1 = 0.0, colL_x2 = 0.0, colL_y1 = 0.0, colL_y2 = 0.0;
    double colL_x1_dry = 0.0, colL_x2_dry = 0.0, colL_y1_dry = 0.0, colL_y2_dry = 0.0;
    double colL_b0 = 1.0, colL_b1 = 0.0, colL_b2 = 0.0, colL_a1 = 0.0, colL_a2 = 0.0;

    // Color (High Shelf @ 12kHz)
    double colH_x1 = 0.0, colH_x2 = 0.0, colH_y1 = 0.0, colH_y2 = 0.0;
    double colH_x1_dry = 0.0, colH_x2_dry = 0.0, colH_y1_dry = 0.0, colH_y2_dry = 0.0;
    double colH_b0 = 1.0, colH_b1 = 0.0, colH_b2 = 0.0, colH_a1 = 0.0, colH_a2 = 0.0;

    // Air (High Shelf @ 5kHz)
    double air_x1 = 0.0, air_x2 = 0.0, air_y1 = 0.0, air_y2 = 0.0;
    double air_x1_dry = 0.0, air_x2_dry = 0.0, air_y1_dry = 0.0, air_y2_dry = 0.0;
    double air_b0 = 1.0, air_b1 = 0.0, air_b2 = 0.0, air_a1 = 0.0, air_a2 = 0.0;
};