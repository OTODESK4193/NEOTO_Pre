#pragma execution_character_set("utf-8")
#include "AnalyzerScreen.h"

AnalyzerScreen::AnalyzerScreen(NeotoPreAudioProcessor& p) : audioProcessor(p)
{
    double initialFs = 44100.0;
    virtualIn_Nickel.prepare(initialFs); virtualIn_Steel.prepare(initialFs); virtualIn_Iron.prepare(initialFs); virtualIn_Amorphous.prepare(initialFs);
    virtualIn_Carnhill.prepare(initialFs); virtualIn_Cinemag.prepare(initialFs); // ★ 追加

    virtualPreamp_API.prepare(initialFs); virtualPreamp_Neve.prepare(initialFs); virtualPreamp_Tube.prepare(initialFs);
    virtualPreamp_SSL.prepare(initialFs); virtualPreamp_Modern1.prepare(initialFs); virtualPreamp_Modern2.prepare(initialFs);

    virtualPreamp_API.setAnalyzerMode(true); virtualPreamp_Neve.setAnalyzerMode(true); virtualPreamp_Tube.setAnalyzerMode(true);
    virtualPreamp_SSL.setAnalyzerMode(true); virtualPreamp_Modern1.setAnalyzerMode(true); virtualPreamp_Modern2.setAnalyzerMode(true);

    virtualOut_Nickel.prepare(initialFs); virtualOut_Steel.prepare(initialFs); virtualOut_Iron.prepare(initialFs); virtualOut_Amorphous.prepare(initialFs);
    virtualOut_Carnhill.prepare(initialFs); virtualOut_Cinemag.prepare(initialFs); // ★ 追加

    virtualOut_Nickel.setAnalyzerMode(true); virtualOut_Steel.setAnalyzerMode(true); virtualOut_Iron.setAnalyzerMode(true); virtualOut_Amorphous.setAnalyzerMode(true);
    virtualOut_Carnhill.setAnalyzerMode(true); virtualOut_Cinemag.setAnalyzerMode(true); // ★ 追加

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
        for (int i = 0; i < block1; ++i) circularBuffer[static_cast<size_t>(writePos++)] = buffer[static_cast<size_t>(start1) + static_cast<size_t>(i)];
        for (int i = 0; i < block2; ++i) circularBuffer[static_cast<size_t>(writePos++)] = buffer[static_cast<size_t>(start2) + static_cast<size_t>(i)];

        fifo.finishedRead(numReady);

        std::copy(circularBuffer.begin(), circularBuffer.end(), spectrumFftData.begin());
        std::fill(spectrumFftData.begin() + 8192, spectrumFftData.end(), 0.0f);

        window.multiplyWithWindowingTable(spectrumFftData.data(), 8192);
        fft.performFrequencyOnlyForwardTransform(spectrumFftData.data());

        for (int i = 1; i < 4096; ++i) {
            float mag = spectrumFftData[static_cast<size_t>(i)];
            if (i > 1 && i < 4095) {
                mag = (spectrumFftData[static_cast<size_t>(i - 1)] + mag * 2.0f + spectrumFftData[static_cast<size_t>(i + 1)]) * 0.25f;
            }
            if (mag > smoothedSpectrum[static_cast<size_t>(i)]) smoothedSpectrum[static_cast<size_t>(i)] = mag;
            else smoothedSpectrum[static_cast<size_t>(i)] = smoothedSpectrum[static_cast<size_t>(i)] * 0.85f + mag * 0.15f;
        }

        auto bounds = getLocalBounds().toFloat();
        spectrumPath.clear();
        bool firstPoint = true;
        double currentFs = audioProcessor.getSampleRate();
        if (currentFs <= 0.0) currentFs = 44100.0;

        for (float x = 0; x <= bounds.getWidth(); x += 1.0f) {
            float freq = getFrequencyForPosition(x, bounds.getWidth());
            if (freq < 5.0f || freq > 20000.0f) continue;

            float binExact = freq * 8192.0f / static_cast<float>(currentFs);
            int bin1 = static_cast<int>(binExact);
            int bin2 = std::min(bin1 + 1, 4095);
            float frac = binExact - static_cast<float>(bin1);

            float mag = smoothedSpectrum[static_cast<size_t>(bin1)] * (1.0f - frac) + smoothedSpectrum[static_cast<size_t>(bin2)] * frac;
            float db = juce::Decibels::gainToDecibels(mag) - 36.0f;
            float y = juce::jmap(db, -90.0f, 36.0f, bounds.getHeight(), 0.0f);
            y = juce::jlimit(0.0f, bounds.getHeight(), y);

            if (firstPoint) {
                spectrumPath.startNewSubPath(x, bounds.getHeight());
                spectrumPath.lineTo(x, y);
                firstPoint = false;
            }
            else spectrumPath.lineTo(x, y);
        }
        if (!firstPoint) {
            spectrumPath.lineTo(bounds.getRight(), bounds.getHeight());
            spectrumPath.closeSubPath();
        }
    }
}

void AnalyzerScreen::generateEQCurve()
{
    int inIdx = (int)audioProcessor.apvts.getRawParameterValue("in_trans_type")->load();
    int preIdx = (int)audioProcessor.apvts.getRawParameterValue("preamp_model")->load();
    int outIdx = (int)audioProcessor.apvts.getRawParameterValue("out_trans_type")->load();

    float drive = audioProcessor.apvts.getRawParameterValue("drive")->load();
    float charac = audioProcessor.apvts.getRawParameterValue("character")->load();
    float asym = audioProcessor.apvts.getRawParameterValue("asymmetry")->load();
    float color = audioProcessor.apvts.getRawParameterValue("color")->load();
    float air = audioProcessor.apvts.getRawParameterValue("air")->load();
    float age = audioProcessor.apvts.getRawParameterValue("age")->load();

    double currentFs = audioProcessor.getSampleRate();
    if (currentFs <= 0.0) currentFs = 44100.0;

    virtualIn_Nickel.prepare(currentFs); virtualIn_Steel.prepare(currentFs); virtualIn_Iron.prepare(currentFs); virtualIn_Amorphous.prepare(currentFs);
    virtualIn_Carnhill.prepare(currentFs); virtualIn_Cinemag.prepare(currentFs);

    virtualPreamp_API.prepare(currentFs); virtualPreamp_Neve.prepare(currentFs); virtualPreamp_Tube.prepare(currentFs); virtualPreamp_SSL.prepare(currentFs);
    virtualPreamp_Modern1.prepare(currentFs); virtualPreamp_Modern2.prepare(currentFs);

    virtualOut_Nickel.prepare(currentFs); virtualOut_Steel.prepare(currentFs); virtualOut_Iron.prepare(currentFs); virtualOut_Amorphous.prepare(currentFs);
    virtualOut_Carnhill.prepare(currentFs); virtualOut_Cinemag.prepare(currentFs);

    for (int i = 0; i < 8192; ++i) {
        float s = 0.0f;
        if (inIdx == 1) s = virtualIn_Nickel.processSample(s);
        else if (inIdx == 2) s = virtualIn_Steel.processSample(s);
        else if (inIdx == 3) s = virtualIn_Iron.processSample(s);
        else if (inIdx == 4) s = virtualIn_Amorphous.processSample(s);
        else if (inIdx == 5) s = virtualIn_Carnhill.processSample(s);
        else if (inIdx == 6) s = virtualIn_Cinemag.processSample(s);

        if (preIdx == 0) s = virtualPreamp_API.processSample(s, drive, charac, asym, age);
        else if (preIdx == 1) s = virtualPreamp_Neve.processSample(s, drive, charac, asym, age);
        else if (preIdx == 2) s = virtualPreamp_Tube.processSample(s, drive, charac, asym, age);
        else if (preIdx == 3) s = virtualPreamp_SSL.processSample(s, drive, charac, asym, age);
        else if (preIdx == 4) s = virtualPreamp_Modern1.processSample(s, drive, charac, asym, age);
        else if (preIdx == 5) s = virtualPreamp_Modern2.processSample(s, drive, charac, asym, age);

        if (outIdx == 1) s = virtualOut_Nickel.processSample(s, color, air, age);
        else if (outIdx == 2) s = virtualOut_Steel.processSample(s, color, air, age);
        else if (outIdx == 3) s = virtualOut_Iron.processSample(s, color, air, age);
        else if (outIdx == 4) s = virtualOut_Amorphous.processSample(s, color, air, age);
        else if (outIdx == 5) s = virtualOut_Carnhill.processSample(s, color, air, age);
        else if (outIdx == 6) s = virtualOut_Cinemag.processSample(s, color, air, age);
    }

    std::fill(eqFftData.begin(), eqFftData.end(), 0.0f);

    // ★ 防御的プログラミング: 大信号による非線形サチュレーション・DCシフトを回避し、純粋な小信号周波数特性を取得する
    const float impulseLevel = 0.01f;
    eqFftData[0] = impulseLevel;

    for (int i = 0; i < 8192; ++i) {
        float s = eqFftData[static_cast<size_t>(i)];

        if (inIdx == 1) s = virtualIn_Nickel.processSample(s);
        else if (inIdx == 2) s = virtualIn_Steel.processSample(s);
        else if (inIdx == 3) s = virtualIn_Iron.processSample(s);
        else if (inIdx == 4) s = virtualIn_Amorphous.processSample(s);
        else if (inIdx == 5) s = virtualIn_Carnhill.processSample(s);
        else if (inIdx == 6) s = virtualIn_Cinemag.processSample(s);

        if (preIdx == 0) s = virtualPreamp_API.processSample(s, drive, charac, asym, age);
        else if (preIdx == 1) s = virtualPreamp_Neve.processSample(s, drive, charac, asym, age);
        else if (preIdx == 2) s = virtualPreamp_Tube.processSample(s, drive, charac, asym, age);
        else if (preIdx == 3) s = virtualPreamp_SSL.processSample(s, drive, charac, asym, age);
        else if (preIdx == 4) s = virtualPreamp_Modern1.processSample(s, drive, charac, asym, age);
        else if (preIdx == 5) s = virtualPreamp_Modern2.processSample(s, drive, charac, asym, age);

        if (outIdx == 1) s = virtualOut_Nickel.processSample(s, color, air, age);
        else if (outIdx == 2) s = virtualOut_Steel.processSample(s, color, air, age);
        else if (outIdx == 3) s = virtualOut_Iron.processSample(s, color, air, age);
        else if (outIdx == 4) s = virtualOut_Amorphous.processSample(s, color, air, age);
        else if (outIdx == 5) s = virtualOut_Carnhill.processSample(s, color, air, age);
        else if (outIdx == 6) s = virtualOut_Cinemag.processSample(s, color, air, age);

        eqFftData[static_cast<size_t>(i)] = s;
    }

    fft.performRealOnlyForwardTransform(eqFftData.data());

    std::array<float, 4096> eqMag = { 0.0f };
    const float delaySamples = 2.0f;
    const float restoreGain = 1.0f / impulseLevel; // ★ 下げたインパルスのゲインを復元

    for (int i = 1; i < 4096; ++i) {
        float re = eqFftData[static_cast<size_t>(i) * 2] * restoreGain;
        float im = eqFftData[static_cast<size_t>(i) * 2 + 1] * restoreGain;
        float theta = juce::MathConstants<float>::twoPi * static_cast<float>(i) * delaySamples / 8192.0f;
        float cosTheta = std::cos(theta);
        float sinTheta = std::sin(theta);

        float reFlat = re * cosTheta - im * sinTheta;
        float imFlat = re * sinTheta + im * cosTheta;
        eqMag[static_cast<size_t>(i)] = std::sqrt(reFlat * reFlat + imFlat * imFlat);
    }

    auto bounds = getLocalBounds().toFloat();
    eqCurvePath.clear();
    bool firstPoint = true;

    for (float x = 0; x <= bounds.getWidth(); x += 1.0f) {
        float freq = getFrequencyForPosition(x, bounds.getWidth());
        if (freq < 5.0f || freq > 22050.0f) continue;

        float binExact = freq * 8192.0f / static_cast<float>(currentFs);
        int bin1 = static_cast<int>(binExact);
        int bin2 = std::min(bin1 + 1, 4095);
        float frac = binExact - static_cast<float>(bin1);

        float mag = eqMag[static_cast<size_t>(bin1)] * (1.0f - frac) + eqMag[static_cast<size_t>(bin2)] * frac;
        float db = juce::Decibels::gainToDecibels(mag) - juce::Decibels::gainToDecibels(1.0f);
        float y = juce::jmap(db, -24.0f, 24.0f, bounds.getHeight(), 0.0f);
        y = juce::jlimit(0.0f, bounds.getHeight(), y);

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
    float charac = audioProcessor.apvts.getRawParameterValue("character")->load();
    float asym = audioProcessor.apvts.getRawParameterValue("asymmetry")->load();
    int outIdx = (int)audioProcessor.apvts.getRawParameterValue("out_trans_type")->load();
    float color = audioProcessor.apvts.getRawParameterValue("color")->load();

    float charNormalized = charac / 100.0f;
    float mixEven = charNormalized;
    float mixOdd = 1.0f - charNormalized;
    float asymNormalized = asym / 100.0f;
    float totalEvenDrive = std::clamp(mixEven + asymNormalized, 0.0f, 1.0f);
    float driveFactor = drive / 100.0f;
    float colorCurve = std::pow(color / 100.0f, 3.0f);

    float evenLvl = driveFactor * totalEvenDrive * 0.8f;
    float oddLvl = driveFactor * mixOdd * 0.8f;

    if (outIdx == 1) { oddLvl += 0.001f + (colorCurve * 2.0f); evenLvl += 0.0005f + (colorCurve * 0.4f); }
    else if (outIdx == 2) { oddLvl += 0.015f + colorCurve * 0.8f; evenLvl += 0.005f + colorCurve * 0.6f; }
    else if (outIdx == 3) { oddLvl += 0.015f + colorCurve * 1.3f; evenLvl += 0.005f + colorCurve * 0.3f; }
    else if (outIdx == 4) { oddLvl += (colorCurve * colorCurve) * 2.5f; }
    else if (outIdx == 5) { evenLvl += 0.01f + colorCurve * 1.5f; oddLvl += 0.005f + colorCurve * 0.5f; } // Carnhill (Even dominant)
    else if (outIdx == 6) { oddLvl += 0.005f + colorCurve * 1.0f; evenLvl += 0.01f + colorCurve * 1.2f; } // Cinemag (Smooth even)

    evenLvl = std::clamp(evenLvl, 0.0f, 1.5f);
    oddLvl = std::clamp(oddLvl, 0.0f, 1.5f);

    audioProcessor.harmonicLevels[0].store(100.0f);
    audioProcessor.harmonicLevels[1].store(evenLvl * 45.0f);
    audioProcessor.harmonicLevels[3].store(evenLvl * 20.0f);
    audioProcessor.harmonicLevels[5].store(evenLvl * 10.0f);
    audioProcessor.harmonicLevels[2].store(oddLvl * 35.0f);
    audioProcessor.harmonicLevels[4].store(oddLvl * 15.0f);
    audioProcessor.harmonicLevels[6].store(oddLvl * 8.0f);
}

float AnalyzerScreen::getPositionForFrequency(float freq, float width)
{
    const float minFreq = 5.0f; const float maxFreq = 20000.0f;
    float normalized = std::log10(freq / minFreq) / std::log10(maxFreq / minFreq);
    return normalized * width;
}

float AnalyzerScreen::getFrequencyForPosition(float x, float width)
{
    const float minFreq = 5.0f; const float maxFreq = 20000.0f;
    float normalized = x / width;
    return minFreq * std::pow(maxFreq / minFreq, normalized);
}

void AnalyzerScreen::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour(juce::Colour(0xff0a0a0c));
    g.fillRoundedRectangle(bounds, 6.0f);
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.drawRoundedRectangle(bounds.reduced(1.0f), 6.0f, 2.0f);

    g.setColour(juce::Colour(0xff1c2a30));
    std::vector<float> freqs = { 5, 10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 };
    for (float freq : freqs) {
        float x = getPositionForFrequency(freq, bounds.getWidth());
        g.drawVerticalLine(static_cast<int>(x), 0.0f, bounds.getHeight());
    }

    g.setFont(10.0f);
    for (int i = 1; i <= 7; ++i) {
        float y = bounds.getHeight() * ((float)i / 8.0f);
        g.setColour(juce::Colour(0xff1c2a30));
        g.drawHorizontalLine(static_cast<int>(y), 0.0f, bounds.getWidth());
        float db = juce::jmap(y, 0.0f, bounds.getHeight(), 24.0f, -24.0f);
        g.setColour(juce::Colours::grey.withAlpha(0.7f));
        g.drawText(juce::String(std::round(db)) + " dB", 4, static_cast<int>(y) - 14, 40, 12, juce::Justification::left);
    }

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

    g.setColour(juce::Colours::white);
    g.strokePath(eqCurvePath, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved));

    g.setColour(juce::Colours::lightgrey);
    std::vector<juce::String> labels = { "5", "10", "20", "50", "100", "200", "500", "1k", "2k", "5k", "10k", "20k" };
    for (size_t i = 0; i < freqs.size(); ++i) {
        float x = getPositionForFrequency(freqs[i], bounds.getWidth());
        g.drawText(labels[i], static_cast<int>(x) + 2, static_cast<int>(bounds.getHeight()) - 16, 30, 12, juce::Justification::left);
    }
}

void AnalyzerScreen::resized() {}