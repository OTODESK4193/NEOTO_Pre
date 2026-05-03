#include "VerticalMeter.h"

VerticalMeter::VerticalMeter() { startTimerHz(60); }
VerticalMeter::~VerticalMeter() { stopTimer(); }

void VerticalMeter::setLevelDb(float newLevelDb)
{
    targetLevelDb = juce::jlimit(-60.0f, 6.0f, newLevelDb);
    if (targetLevelDb > peakHoldDb) {
        peakHoldDb = targetLevelDb;
        peakHoldTimer = 60;
    }
}

void VerticalMeter::timerCallback()
{
    if (targetLevelDb > currentLevelDb) currentLevelDb = targetLevelDb;
    else {
        currentLevelDb -= 1.5f;
        if (currentLevelDb < targetLevelDb) currentLevelDb = targetLevelDb;
    }

    if (peakHoldTimer > 0) peakHoldTimer--;
    else {
        peakHoldDb -= 0.5f;
        if (peakHoldDb < -60.0f) peakHoldDb = -60.0f;
    }
    repaint();
}

void VerticalMeter::resized()
{
    auto bounds = getLocalBounds().toFloat().withTrimmedTop(25.0f);
    // パステル調のグラデーション (Soft Pink -> Pastel Blue)
    gradient = juce::ColourGradient(juce::Colour(0xffff88a3), bounds.getTopLeft(), juce::Colour(0xff88aaff), bounds.getBottomLeft(), false);
    gradient.addColour(0.3, juce::Colour(0xff88eeff)); // Soft Cyan
    gradient.addColour(0.6, juce::Colour(0xff88ffaa)); // Mint Green
    gradient.addColour(0.85, juce::Colour(0xffffee88)); // Soft Yellow
}

void VerticalMeter::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // 上部の数値表示
    auto textArea = bounds.removeFromTop(20.0f);
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    juce::String peakText = (peakHoldDb > -59.0f) ? juce::String(peakHoldDb, 1) : "-inf";
    g.drawText(peakText, textArea, juce::Justification::centredBottom);

    bounds.removeFromTop(5.0f); // 隙間

    // メーター背景
    g.setColour(juce::Colour(0xff111111));
    g.fillRoundedRectangle(bounds, 4.0f);

    auto dbToY = [&bounds](float db) {
        float normalized = juce::jmap(db, -60.0f, 0.0f, 0.0f, 1.0f);
        return bounds.getHeight() * (1.0f - juce::jlimit(0.0f, 1.0f, normalized));
        };

    float yPos = dbToY(currentLevelDb);
    juce::Rectangle<float> meterFill(bounds.getX(), yPos, bounds.getWidth(), bounds.getHeight() - yPos);
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(meterFill, 4.0f);

    if (peakHoldDb > -59.0f) {
        float peakY = dbToY(peakHoldDb);
        g.setColour(juce::Colours::white.withAlpha(0.8f));
        g.fillRect(bounds.getX(), peakY, bounds.getWidth(), 2.0f);
    }
}