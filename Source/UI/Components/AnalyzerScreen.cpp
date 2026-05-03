#include "AnalyzerScreen.h"

AnalyzerScreen::AnalyzerScreen()
{
}

AnalyzerScreen::~AnalyzerScreen()
{
}

float AnalyzerScreen::getPositionForFrequency(float freq, float width)
{
    const float minFreq = 20.0f;
    const float maxFreq = 20000.0f;
    float normalized = std::log10(freq / minFreq) / std::log10(maxFreq / minFreq);
    return normalized * width;
}

void AnalyzerScreen::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // LCD背景（ダークグレー/ブラック）
    g.setColour(juce::Colour(0xff0a0a0c));
    g.fillRoundedRectangle(bounds, 6.0f);

    // ディスプレイのインナーシャドウ（立体感）
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.drawRoundedRectangle(bounds.reduced(1.0f), 6.0f, 2.0f);

    // グリッド線の描画 (20Hz ~ 20kHz)
    g.setColour(juce::Colour(0xff1c2a30)); // ほのかな青緑色のグリッド

    std::vector<float> freqs = { 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 };
    for (float freq : freqs)
    {
        float x = getPositionForFrequency(freq, bounds.getWidth());
        g.drawVerticalLine(static_cast<int>(x), 0.0f, bounds.getHeight());
    }

    // 水平グリッド線 (dB用・仮)
    int numHorizontalLines = 5;
    for (int i = 1; i < numHorizontalLines; ++i)
    {
        float y = bounds.getHeight() * ((float)i / numHorizontalLines);
        g.drawHorizontalLine(static_cast<int>(y), 0.0f, bounds.getWidth());
    }

    // 周波数ラベルの描画
    g.setColour(juce::Colours::darkgrey);
    g.setFont(12.0f);
    g.drawText("100", static_cast<int>(getPositionForFrequency(100, bounds.getWidth())) + 4, static_cast<int>(bounds.getHeight()) - 18, 40, 15, juce::Justification::left);
    g.drawText("1k", static_cast<int>(getPositionForFrequency(1000, bounds.getWidth())) + 4, static_cast<int>(bounds.getHeight()) - 18, 40, 15, juce::Justification::left);
    g.drawText("10k", static_cast<int>(getPositionForFrequency(10000, bounds.getWidth())) + 4, static_cast<int>(bounds.getHeight()) - 18, 40, 15, juce::Justification::left);

    // ※今後のステップで、ここに `Path` を使ってEQカーブや倍音を描画するコードを追加します。
}