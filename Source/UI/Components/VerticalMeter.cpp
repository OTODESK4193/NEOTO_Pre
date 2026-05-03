#include "VerticalMeter.h"

VerticalMeter::VerticalMeter()
{
    // 60FPSで滑らかにアニメーションさせるためのタイマー
    startTimerHz(60);
}

VerticalMeter::~VerticalMeter()
{
    stopTimer();
}

void VerticalMeter::setLevelDb(float newLevelDb)
{
    targetLevelDb = juce::jlimit(-60.0f, 6.0f, newLevelDb);

    // ピークホールドの更新
    if (targetLevelDb > peakHoldDb) {
        peakHoldDb = targetLevelDb;
        peakHoldTimer = 60; // 約1秒間保持 (60fps * 1)
    }
}

void VerticalMeter::timerCallback()
{
    // メーターの滑らかな追従（アタックは速く、リリースは遅く）
    if (targetLevelDb > currentLevelDb) {
        currentLevelDb = targetLevelDb; // アタックは即座に反応
    }
    else {
        currentLevelDb -= 1.5f; // 滑らかなリリース
        if (currentLevelDb < targetLevelDb) currentLevelDb = targetLevelDb;
    }

    // ピークホールドの減衰
    if (peakHoldTimer > 0) {
        peakHoldTimer--;
    }
    else {
        peakHoldDb -= 0.5f;
        if (peakHoldDb < -60.0f) peakHoldDb = -60.0f;
    }

    repaint();
}

void VerticalMeter::resized()
{
    // グラデーションの作成（上：赤、中央：黄、下：緑）
    auto bounds = getLocalBounds().toFloat();
    gradient = juce::ColourGradient(juce::Colours::red, bounds.getTopLeft(),
        juce::Colours::green, bounds.getBottomLeft(), false);
    gradient.addColour(0.3, juce::Colours::yellow);
}

void VerticalMeter::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // 背景（暗いメーター枠）
    g.setColour(juce::Colour(0xff111111));
    g.fillRoundedRectangle(bounds, 4.0f);

    // dBを高さの割合に変換 (-60dB 〜 0dB)
    auto dbToY = [&bounds](float db) {
        float normalized = juce::jmap(db, -60.0f, 0.0f, 0.0f, 1.0f);
        normalized = juce::jlimit(0.0f, 1.0f, normalized);
        return bounds.getHeight() * (1.0f - normalized);
        };

    // 現在のレベルの描画
    float yPos = dbToY(currentLevelDb);
    juce::Rectangle<float> meterFill(bounds.getX(), yPos, bounds.getWidth(), bounds.getHeight() - yPos);
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(meterFill, 4.0f);

    // ピークホールド線の描画
    if (peakHoldDb > -59.0f) {
        float peakY = dbToY(peakHoldDb);
        g.setColour(juce::Colours::white.withAlpha(0.8f));
        g.fillRect(bounds.getX(), peakY, bounds.getWidth(), 2.0f);
    }
}