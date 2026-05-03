#include "NickelTransformer.h"
#include <cmath>
#include <algorithm>

void NickelTransformer::prepare(double sampleRate)
{
    fs = sampleRate;
    lastInputADAA = 0.0f;
}

float NickelTransformer::fx(float x) const
{
    // 入力トランスのハードクリッピング（閾値が高く、超えると急激に潰れる）[cite: 5]
    // 簡易的な多項式ハードクリッパー（閾値 x = 1.5）
    if (x > 1.5f) return 1.5f;
    if (x < -1.5f) return -1.5f;
    return x;
}

float NickelTransformer::F1(float x) const
{
    // 上記ハードクリッパーの第1次不定積分
    if (x > 1.5f) return 1.5f * x - 1.125f; // (1.5 * 1.5)/2
    if (x < -1.5f) return -1.5f * x - 1.125f;
    return 0.5f * x * x;
}

float NickelTransformer::processSample(float input)
{
    // 通常のマイクレベルではリニア動作、過大入力時のみクリップする[cite: 5]
    float softclipOut = 0.0f;
    float dx = input - lastInputADAA;
    const float eps = 1e-5f;

    if (std::abs(dx) < eps)
    {
        // イルコンディション回避
        float x_mid = (input + lastInputADAA) * 0.5f;
        softclipOut = fx(x_mid);
    }
    else
    {
        // 1次ADAAによるエイリアシング除去[cite: 6]
        softclipOut = (F1(input) - F1(lastInputADAA)) / dx;
    }
    lastInputADAA = input;

    return softclipOut;
}