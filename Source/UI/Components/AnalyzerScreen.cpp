#include "AnalyzerScreen.h"

AnalyzerScreen::AnalyzerScreen(NeotoPreAudioProcessor& p) : audioProcessor(p)
{
    virtualPreamp_API.prepare(44100.0);
    virtualOut_Steel.prepare(44100.0);
    startTimerHz(30);
}

AnalyzerScreen::~AnalyzerScreen() { stopTimer(); }

void AnalyzerScreen::timerCallback()
{
    drawNextFrameOfSpectrum();
    generateEQCurve();
    calculateHarmonics();
    repaint();
}

void AnalyzerScreen::drawNextFrameOfSpectrum()
{
    auto& fifo = audioProcessor.audioFifo;
    auto& buffer = audioProcessor.audioFifoBuffer;

    int numReady = fifo.getNumReady();
    if (numReady > 0) {
        numReady = std::min(numReady, 8192);

        int start1, block1, start2, block2;
        fifo.prepareToRead(numReady, start1, block1, start2, block2);

        std::copy(circularBuffer.begin() + numReady, circularBuffer.end(), circularBuffer.begin());

        int writePos = 8192 - numReady;
        for (int i = 0; i < block1; ++i) circularBuffer[writePos++] = buffer[(size_t)(start1 + i)];
        for (int i = 0; i < block2; ++i) circularBuffer[writePos++] = buffer[(size_t)(start2 + i)];

        fifo.finishedRead(numReady);

        std::copy(circularBuffer.begin(), circularBuffer.end(), spectrumFftData.begin());
        std::fill(spectrumFftData.begin() + 8192, spectrumFftData.end(), 0.0f);

        window.multiplyWithWindowingTable(spectrumFftData.data(), 8192);
        fft.performFrequencyOnlyForwardTransform(spectrumFftData.data());

        auto bounds = getLocalBounds().toFloat();
        spectrumPath.clear();
        bool firstPoint = true;

        for (int i = 1; i < 4096; ++i) {
            float freq = (i * 44100.0f) / 8192.0f;
            if (freq < 20.0f || freq > 20000.0f) continue;

            float mag = spectrumFftData[i];

            if (i > 1 && i < 4095) {
                mag = (spectrumFftData[i - 1] + mag * 2.0f + spectrumFftData[i + 1]) * 0.25f;
            }

            if (mag > smoothedSpectrum[i]) smoothedSpectrum[i] = mag;
            else smoothedSpectrum[i] = smoothedSpectrum[i] * 0.85f + mag * 0.15f;

            float db = juce::Decibels::gainToDecibels(smoothedSpectrum[i]) - 18.0f;
            float y = juce::jmap(db, -90.0f, 36.0f, bounds.getHeight(), 0.0f);
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

    for (int i = 0; i < 8192; ++i) {
        float s = eqFftData[i];
        if (preIdx == 0) s = virtualPreamp_API.processSample(s, drive, charac, asym, age);
        if (outIdx == 2) s = virtualOut_Steel.processSample(s, color, air, age);
        eqFftData[i] = s;
    }

    // ★ 変更: 位相情報を保持するため、複素数出力用のRealOnly Forward Transformを使用
    fft.performRealOnlyForwardTransform(eqFftData.data());

    auto bounds = getLocalBounds().toFloat();
    eqCurvePath.clear();
    bool firstPoint = true;

    // ★ 1次ADAAの遅延量 (0.5サンプル)
    const float delaySamples = 0.5f;

    for (int i = 1; i < 4096; ++i) {
        float freq = (i * 44100.0f) / 8192.0f;
        if (freq < 20.0f || freq > 20000.0f) continue;

        // ★ 複素スペクトルからの実部・虚部の取得
        float re = eqFftData[i * 2];
        float im = eqFftData[i * 2 + 1];

        // ★ 位相逆回転の適用 (Phase Rotation Cancellation)
        // theta = 2 * pi * k * D / N
        float theta = juce::MathConstants<float>::twoPi * static_cast<float>(i) * delaySamples / 8192.0f;
        float cosTheta = std::cos(theta);
        float sinTheta = std::sin(theta);

        // 複素数乗算: (re + j*im) * (cosTheta + j*sinTheta)
        float reFlat = re * cosTheta - im * sinTheta;
        float imFlat = re * sinTheta + im * cosTheta;

        // 相殺後の正確な振幅(Magnitude)を算出
        float mag = std::sqrt(reFlat * reFlat + imFlat * imFlat);

        float db = juce::Decibels::gainToDecibels(mag) - juce::Decibels::gainToDecibels(1.0f);
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

    audioProcessor.harmonicLevels[0].store(100.0f);
    audioProcessor.harmonicLevels[1].store((drive / 100.0f) * (asym / 100.0f) * 45.0f);
    audioProcessor.harmonicLevels[2].store((drive / 100.0f) * 35.0f);
    audioProcessor.harmonicLevels[3].store((drive / 100.0f) * (asym / 100.0f) * 20.0f);
    audioProcessor.harmonicLevels[4].store((drive / 100.0f) * 15.0f);
    audioProcessor.harmonicLevels[5].store((drive / 100.0f) * (asym / 100.0f) * 10.0f);
    audioProcessor.harmonicLevels[6].store((drive / 100.0f) * 8.0f);
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

    // ★ グリッド線の描画
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

    // スペクトラム描画
    if (!spectrumPath.isEmpty()) {
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

    // EQカーブ描画
    g.setColour(juce::Colours::white);
    g.strokePath(eqCurvePath, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved));

    // ★ 周波数テキストの描画 (全てのグリッドに連動・グラデーションの背面に隠れないように最前面へ)
    g.setColour(juce::Colours::lightgrey);
    g.setFont(10.0f); // 密集するためフォントサイズを微調整
    std::vector<juce::String> labels = { "20", "50", "100", "200", "500", "1k", "2k", "5k", "10k", "20k" };
    for (size_t i = 0; i < freqs.size(); ++i) {
        float x = getPositionForFrequency(freqs[i], bounds.getWidth());
        g.drawText(labels[i], static_cast<int>(x) + 2, static_cast<int>(bounds.getHeight()) - 16, 30, 12, juce::Justification::left);
    }
}

void AnalyzerScreen::resized() {}