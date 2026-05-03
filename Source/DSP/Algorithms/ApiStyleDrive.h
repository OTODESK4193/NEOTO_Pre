#pragma once
#include <JuceHeader.h>

class ApiStyleDrive
{
public:
    ApiStyleDrive() = default;
    ~ApiStyleDrive() = default;

    void prepare(double sampleRate);
    float processSample(float input, float driveParam, float charParam, float asymParam, float ageParam);

private:
    double fx(double x) const;
    double F1(double x) const;

    double fs = 44100.0;

    // ADAA & 積分器の状態変数
    double integratorState = 0.0;
    double lastInputADAA = 0.0;
    double lastSoftclipOut = 0.0;

    // Age (Sag) 用のエンベロープフォロワー状態変数
    double envState = 0.0;

    // キャッシュ変数
    float lastDriveParam = -1.0f;
    float lastCharParam = -1.0f;
    float lastAsymParam = -1.0f;
    float lastAgeParam = -1.0f;

    double gain = 1.0;
    double makeUp = 1.0;
    double alpha = 0.0;
    double oneMinusAlpha = 1.0;
    double bias = 0.0;
    double fxBias = 0.0;

    // Sag制御用係数
    double envAttackCoef = 0.0;
    double envReleaseCoef = 0.0;
    double sagRatio = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ApiStyleDrive)
};