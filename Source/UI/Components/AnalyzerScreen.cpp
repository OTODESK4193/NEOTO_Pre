#include "AnalyzerScreen.h"

AnalyzerScreen::AnalyzerScreen(NeotoPreAudioProcessor& p) : audioProcessor(p)
{
    virtualPreamp_API.prepare(44100.0);
    virtualOut_Steel.prepare(44100.0);
    startTimerHz(30); // 30FPS
}

AnalyzerScreen::~AnalyzerScreen() { stopTimer(); }

void AnalyzerScreen::timerCallback()
{
    drawNextFrameOfSpectrum();
    generateEQCurve();
    calculateHarmonics(); // 倍音を計算
    repaint();
}

void AnalyzerScreen::drawNextFrameOfSpectrum()
{
    auto& fifo = audioProcessor.audioFifo;
    auto& buffer = audioProcessor.audioFifoBuffer;

    if (fifo.getNumReady() >= 2048) {
        int start1, block1, start2, block2;
        fifo.prepareToRead(2048, start1, block1, start2, block2);

        int outIdx = 0;
        for (int i = 0; i < block1; ++i) spectrumFifo[outIdx++] = buffer[(size_t)(start1 + i)];
        for (int i = 0; i < block2; ++i) spectrumFifo[outIdx++] = buffer[(size_t)(start2 + i)];
        fifo.finishedRead(2048);

        std::copy(spectrumFifo.begin(), spectrumFifo.begin() + 2048, spectrumFftData.begin());
        window.multiplyWithWindowingTable(spectrumFftData.data(), 2048);
        fft.performFrequencyOnlyForwardTransform(spectrumFftData.data());

        auto bounds = getLocalBounds().toFloat();
        spectrumPath.clear();
        bool firstPoint = true;

        for (int i = 1; i < 1024; ++i) {
            float freq = (i * 44100.0f) / 2048.0f;
            if (freq < 20.0f || freq > 20000.0f) continue;

            float mag = spectrumFftData[i];

            // ★ スムージング処理 (アタック速め・リリース滑らか)
            if (mag > smoothedSpectrum[i]) smoothedSpectrum[i] = mag;
            else smoothedSpectrum[i] = smoothedSpectrum[i] * 0.85f + mag * 0.15f;

            // ★ スケーリング調整 (+12dBまで余裕を持たせる)
            float db = juce::Decibels::gainToDecibels(smoothedSpectrum[i]) - 18.0f;
            float y = juce::jmap(db, -90.0f, 12.0f, bounds.getHeight(), 0.0f);
            y = juce::jlimit(0.0f, bounds.getHeight(), y);

            float x = getPositionForFrequency(freq, bounds.getWidth());

            if (firstPoint) {
                spectrumPath.startNewSubPath(x, bounds.getHeight());
                spectrumPath.lineTo(x, y);
                firstPoint = false;
            }
            else {
                spectrumPath.lineTo(x, y);
            }
        }
        if (!firstPoint) {
            spectrumPath.lineTo(bounds.getRight(), bounds.getHeight());
            spectrumPath.closeSubPath();
        }
    }
}

void AnalyzerScreen::generateEQCurve()
{
    std::fill(eqFftData.begin(), eqFftData.end(), 0.0f);
    eqFftData[0] = 1.0f;

    int preIdx = (int)audioProcessor.apvts.getRawParameterValue("preamp_model")->load();
    float drive = audioProcessor.apvts.getRawParameterValue("drive")->load();
    float charac = audioProcessor.apvts.getRawParameterValue("character")->load();
    float asym = audioProcessor.apvts.getRawParameterValue("asymmetry")->load();

    int outIdx = (int)audioProcessor.apvts.getRawParameterValue("out_trans_type")->load();
    float color = audioProcessor.apvts.getRawParameterValue("color")->load();
    float air = audioProcessor.apvts.getRawParameterValue("air")->load();
    float age = audioProcessor.apvts.getRawParameterValue("age")->load();

    for (int i = 0; i < 2048; ++i) {
        float s = eqFftData[i];
        if (preIdx == 0) s = virtualPreamp_API.processSample(s, drive, charac, asym, age);
        if (outIdx == 2) s = virtualOut_Steel.processSample(s, color, air, age);
        eqFftData[i] = s;
    }

    fft.performFrequencyOnlyForwardTransform(eqFftData.data());

    auto bounds = getLocalBounds().toFloat();
    eqCurvePath.clear();
    bool firstPoint = true;

    for (int i = 1; i < 1024; ++i) {
        float freq = (i * 44100.0f) / 2048.0f;
        if (freq < 20.0f || freq > 20000.0f) continue;
        float db = juce::Decibels::gainToDecibels(eqFftData[i]) - juce::Decibels::gainToDecibels(1.0f);
        float y = juce::jmap(db, -24.0f, 24.0f, bounds.getHeight(), 0.0f);
        y = juce::jlimit(0.0f, bounds.getHeight(), y);
        float x = getPositionForFrequency(freq, bounds.getWidth());

        if (firstPoint) {
            eqCurvePath.startNewSubPath(x, y);
            firstPoint = false;
        }
        else eqCurvePath.lineTo(x, y);
    }
}

void AnalyzerScreen::calculateHarmonics()
{
    float drive = audioProcessor.apvts.getRawParameterValue("drive")->load();
    float asym = audioProcessor.apvts.getRawParameterValue("asymmetry")->load();

    // 基音(1st)を100%とし、DriveとAsymmetryで2次〜7次倍音を生成
    audioProcessor.harmonicLevels[0].store(100.0f); // 1st (Fundamental)
    audioProcessor.harmonicLevels[1].store((drive / 100.0f) * (asym / 100.0f) * 40.0f); // 2nd (Even)
    audioProcessor.harmonicLevels[2].store((drive / 100.0f) * 30.0f);                   // 3rd (Odd)
    audioProcessor.harmonicLevels[3].store((drive / 100.0f) * (asym / 100.0f) * 15.0f); // 4th (Even)
    audioProcessor.harmonicLevels[4].store((drive / 100.0f) * 10.0f);                   // 5th (Odd)
    audioProcessor.harmonicLevels[5].store((drive / 100.0f) * (asym / 100.0f) * 5.0f);  // 6th (Even)
    audioProcessor.harmonicLevels[6].store((drive / 100.0f) * 2.0f);                    // 7th (Odd)
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

    g.setColour(juce::Colour(0xff0a0a0c));
    g.fillRoundedRectangle(bounds, 6.0f);
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.drawRoundedRectangle(bounds.reduced(1.0f), 6.0f, 2.0f);

    g.setColour(juce::Colour(0xff1c2a30));
    std::vector<float> freqs = { 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 };
    for (float freq : freqs) {
        float x = getPositionForFrequency(freq, bounds.getWidth());
        g.drawVerticalLine(static_cast<int>(x), 0.0f, bounds.getHeight());
    }
    for (int i = 1; i < 7; ++i) {
        float y = bounds.getHeight() * ((float)i / 7);
        g.drawHorizontalLine(static_cast<int>(y), 0.0f, bounds.getWidth());
    }

    g.setColour(juce::Colours::darkgrey);
    g.setFont(12.0f);
    g.drawText("100", static_cast<int>(getPositionForFrequency(100, bounds.getWidth())) + 4, static_cast<int>(bounds.getHeight()) - 18, 40, 15, juce::Justification::left);
    g.drawText("1k", static_cast<int>(getPositionForFrequency(1000, bounds.getWidth())) + 4, static_cast<int>(bounds.getHeight()) - 18, 40, 15, juce::Justification::left);
    g.drawText("10k", static_cast<int>(getPositionForFrequency(10000, bounds.getWidth())) + 4, static_cast<int>(bounds.getHeight()) - 18, 40, 15, juce::Justification::left);

    // ==============================================================================
    // ★ 1. FLEX風 グラデーション・スペクトラムの描画
    // ==============================================================================
    if (!spectrumPath.isEmpty()) {
        // パステル調の横方向グラデーション
        juce::ColourGradient specGrad(juce::Colour(0xffff88a3), 0, 0, juce::Colour(0xffaa88ff), bounds.getWidth(), 0, false);
        specGrad.addColour(0.2f, juce::Colour(0xffffee88));
        specGrad.addColour(0.4f, juce::Colour(0xff88ffaa));
        specGrad.addColour(0.7f, juce::Colour(0xff88eeff));

        g.setGradientFill(specGrad);
        g.fillPath(spectrumPath);

        juce::ColourGradient fadeGrad(juce::Colours::transparentBlack, 0, bounds.getHeight() * 0.3f, juce::Colour(0xff0a0a0c), 0, bounds.getHeight() * 0.95f, false);
        g.setGradientFill(fadeGrad);
        g.fillRect(bounds);
    }

    // ==============================================================================
    // 2. EQカーブの描画
    // ==============================================================================
    g.setColour(juce::Colours::white);
    g.strokePath(eqCurvePath, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved));
}

void AnalyzerScreen::resized() {}