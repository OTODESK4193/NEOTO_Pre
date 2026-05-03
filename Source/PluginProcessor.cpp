#include "PluginProcessor.h"
#include "PluginEditor.h"

NeotoPreAudioProcessor::NeotoPreAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    ),
    apvts(*this, nullptr, "Parameters", createParameterLayout())
#else
    : apvts(*this, nullptr, "Parameters", createParameterLayout())
#endif
{
    inputGainParam = apvts.getRawParameterValue("input_gain");
    outputGainParam = apvts.getRawParameterValue("output_gain");
    mixParam = apvts.getRawParameterValue("mix");
    listenModeParam = apvts.getRawParameterValue("listen_mode");
    osModeParam = apvts.getRawParameterValue("os_mode");

    inTransParam = apvts.getRawParameterValue("in_trans_type");
    preampModelParam = apvts.getRawParameterValue("preamp_model");
    outTransParam = apvts.getRawParameterValue("out_trans_type");

    driveParam = apvts.getRawParameterValue("drive");
    colorParam = apvts.getRawParameterValue("color");
    charParam = apvts.getRawParameterValue("character");
    asymParam = apvts.getRawParameterValue("asymmetry");
    airParam = apvts.getRawParameterValue("air");
    ageParam = apvts.getRawParameterValue("age");
    analysisTimeParam = apvts.getRawParameterValue("analysis_time");

    for (int i = 0; i < 4; ++i) {
        oversamplers[i] = std::make_unique<juce::dsp::Oversampling<float>>(
            2, i, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true);
    }

    for (int ch = 0; ch < 2; ++ch) {
        inTransEngines[ch][0] = std::make_unique<InputTransformer_None>();
        inTransEngines[ch][1] = std::make_unique<InputTransformer_Nickel>();
        inTransEngines[ch][2] = std::make_unique<InputTransformer_Steel>();
        inTransEngines[ch][3] = std::make_unique<InputTransformer_Iron>();
        inTransEngines[ch][4] = std::make_unique<InputTransformer_Amorphous>();

        preampEngines[ch][0] = std::make_unique<Preamp_API>();
        preampEngines[ch][1] = std::make_unique<Preamp_Neve>();
        preampEngines[ch][2] = std::make_unique<Preamp_Tube>();
        preampEngines[ch][3] = std::make_unique<Preamp_SSL>();
        preampEngines[ch][4] = std::make_unique<Preamp_Modern1>();
        preampEngines[ch][5] = std::make_unique<Preamp_Modern2>();

        outTransEngines[ch][0] = std::make_unique<OutputTransformer_None>();
        outTransEngines[ch][1] = std::make_unique<OutputTransformer_Nickel>();
        outTransEngines[ch][2] = std::make_unique<OutputTransformer_Steel>();
        outTransEngines[ch][3] = std::make_unique<OutputTransformer_Iron>();
        outTransEngines[ch][4] = std::make_unique<OutputTransformer_Amorphous>();
    }
}

NeotoPreAudioProcessor::~NeotoPreAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout NeotoPreAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "input_gain", 1 }, "Input Gain", -24.0f, 24.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "output_gain", 1 }, "Output Gain", -24.0f, 24.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "mix", 1 }, "Mix", 0.0f, 100.0f, 100.0f));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ "listen_mode", 1 }, "Listen Mode", false));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ "os_mode", 1 }, "Oversampling",
        juce::StringArray{ "Off (1x)", "2x", "4x", "8x" }, 1));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ "in_trans_type", 1 }, "Input Transformer",
        juce::StringArray{ "None", "Nickel", "Steel", "Iron", "Amorphous" }, 1));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ "preamp_model", 1 }, "Preamp Model",
        juce::StringArray{ "API Style", "Neve Style", "Vintage Tube", "SSL Modern", "Modern 1", "Modern 2" }, 0));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ "out_trans_type", 1 }, "Output Transformer",
        juce::StringArray{ "None", "Nickel", "Steel", "Iron", "Amorphous" }, 1));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "drive", 1 }, "Drive", 0.0f, 100.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "color", 1 }, "Color", 0.0f, 100.0f, 50.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "character", 1 }, "Character", 0.0f, 100.0f, 50.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "asymmetry", 1 }, "Asymmetry", 0.0f, 100.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "air", 1 }, "Air", 0.0f, 100.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "age", 1 }, "Age", 0.0f, 100.0f, 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ "analysis_time", 1 }, "Analysis Time",
        juce::StringArray{ "1 sec", "3 sec", "5 sec", "10 sec" }, 1));

    return { params.begin(), params.end() };
}

void NeotoPreAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentOsMode = static_cast<int>(osModeParam->load());

    // ★ 巨大な固定長リングバッファの初期化 (プロセスブロック中の動的確保を完全排除)
    for (int i = 0; i < 2; ++i) {
        dryDelayBuffer[i].assign(32768, 0.0f);
        dryDelayWritePos[i] = 0;
    }

    for (auto& al : autoLevels) al.prepare(sampleRate);
    for (auto& os : oversamplers) os->initProcessing(samplesPerBlock);

    for (int i = 0; i < 2; ++i) {
        inputGainSmoother[i].reset(sampleRate, 0.02);
        outputGainSmoother[i].reset(sampleRate, 0.02);
        mixSmoother[i].reset(sampleRate, 0.02);

        inputGainSmoother[i].setCurrentAndTargetValue(juce::Decibels::decibelsToGain(inputGainParam->load()));
        outputGainSmoother[i].setCurrentAndTargetValue(juce::Decibels::decibelsToGain(outputGainParam->load()));
        mixSmoother[i].setCurrentAndTargetValue(mixParam->load() / 100.0f);
    }

    double oversampledRate = sampleRate * (1 << currentOsMode);

    for (int ch = 0; ch < 2; ++ch) {
        for (int i = 0; i < 5; ++i) inTransEngines[ch][i]->prepare(oversampledRate);
        for (int i = 0; i < 6; ++i) preampEngines[ch][i]->prepare(oversampledRate);
        for (int i = 0; i < 5; ++i) outTransEngines[ch][i]->prepare(oversampledRate);

        driveSmoother[ch].reset(oversampledRate, 0.02);
        colorSmoother[ch].reset(oversampledRate, 0.02);
        charSmoother[ch].reset(oversampledRate, 0.02);
        asymSmoother[ch].reset(oversampledRate, 0.02);
        airSmoother[ch].reset(oversampledRate, 0.02);
        ageSmoother[ch].reset(oversampledRate, 0.02);
    }

    inPeakState = 0.0f;
    outPeakState = 0.0f;
    inputPeakDb.store(-60.0f);
    outputPeakDb.store(-60.0f);

    int latency = currentOsMode > 0 ? static_cast<int>(oversamplers[currentOsMode]->getLatencyInSamples()) : 0;
    setLatencySamples(latency);
}

void NeotoPreAudioProcessor::releaseResources()
{
    for (auto& os : oversamplers) os->reset();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool NeotoPreAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()) return false;
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet()) return false;
    return true;
}
#endif

void NeotoPreAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    int numSamples = buffer.getNumSamples();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, numSamples);

    int newOsMode = static_cast<int>(osModeParam->load());
    if (newOsMode != currentOsMode)
    {
        currentOsMode = newOsMode;
        double newRate = currentSampleRate * (1 << currentOsMode);
        for (int ch = 0; ch < 2; ++ch) {
            for (int i = 0; i < 5; ++i) inTransEngines[ch][i]->prepare(newRate);
            for (int i = 0; i < 6; ++i) preampEngines[ch][i]->prepare(newRate);
            for (int i = 0; i < 5; ++i) outTransEngines[ch][i]->prepare(newRate);
            driveSmoother[ch].reset(newRate, 0.02); colorSmoother[ch].reset(newRate, 0.02);
            charSmoother[ch].reset(newRate, 0.02); asymSmoother[ch].reset(newRate, 0.02);
            airSmoother[ch].reset(newRate, 0.02); ageSmoother[ch].reset(newRate, 0.02);
        }
        int latency = currentOsMode > 0 ? static_cast<int>(oversamplers[currentOsMode]->getLatencyInSamples()) : 0;
        setLatencySamples(latency);
    }

    if (triggerAnalyze.exchange(false))
    {
        int timeSelection = static_cast<int>(analysisTimeParam->load());
        float seconds = (timeSelection == 0) ? 1.0f : (timeSelection == 1) ? 3.0f : (timeSelection == 2) ? 5.0f : 10.0f;

        for (int ch = 0; ch < totalNumInputChannels; ++ch) autoLevels[ch].analyzeRMS(seconds);

        float dryEnergyL = autoLevels[0].getLatestDryRMS();
        float wetEnergyL = autoLevels[0].getLatestWetRMS();
        float dryEnergyR = totalNumInputChannels > 1 ? autoLevels[1].getLatestDryRMS() : dryEnergyL;
        float wetEnergyR = totalNumInputChannels > 1 ? autoLevels[1].getLatestWetRMS() : wetEnergyL;

        float totalDryEnergy = dryEnergyL + dryEnergyR;
        float totalWetEnergy = wetEnergyL + wetEnergyR;

        const float lufsOffset = -0.691f;
        float dryLufs = -100.0f;
        float wetLufs = -100.0f;

        if (totalDryEnergy > 1e-10f) dryLufs = 10.0f * std::log10(totalDryEnergy) + lufsOffset;
        if (totalWetEnergy > 1e-10f) wetLufs = 10.0f * std::log10(totalWetEnergy) + lufsOffset;

        latestAnalysisResult.dryRmsL = dryLufs;
        latestAnalysisResult.wetRmsL = wetLufs;
        latestAnalysisResult.dryRmsR = 0.0f;
        latestAnalysisResult.wetRmsR = 0.0f;

        if (totalWetEnergy > 1e-10f && totalDryEnergy > 1e-10f) {
            latestAnalysisResult.suggestedGainDb = dryLufs - wetLufs;
        }
        else {
            latestAnalysisResult.suggestedGainDb = 0.0f;
        }

        hasNewAnalysisResult.store(true);
    }

    float targetInGain = juce::Decibels::decibelsToGain(inputGainParam->load());
    float targetOutGain = juce::Decibels::decibelsToGain(outputGainParam->load());
    float targetMix = mixParam->load() / 100.0f;
    bool isListenDry = listenModeParam->load() > 0.5f;

    int currentInTransIdx = static_cast<int>(inTransParam->load());
    int currentPreampIdx = static_cast<int>(preampModelParam->load());
    int currentOutTransIdx = static_cast<int>(outTransParam->load());

    for (int i = 0; i < 2; ++i) {
        inputGainSmoother[i].setTargetValue(targetInGain);
        outputGainSmoother[i].setTargetValue(targetOutGain);
        mixSmoother[i].setTargetValue(targetMix);
        driveSmoother[i].setTargetValue(driveParam->load());
        colorSmoother[i].setTargetValue(colorParam->load());
        charSmoother[i].setTargetValue(charParam->load());
        asymSmoother[i].setTargetValue(asymParam->load());
        airSmoother[i].setTargetValue(airParam->load());
        ageSmoother[i].setTargetValue(ageParam->load());
    }

    float currentBlockInPeak = 0.0f;
    float currentBlockOutPeak = 0.0f;

    // [Phase 1] Input Gain & リングバッファへの記録
    for (int channel = 0; channel < totalNumInputChannels; ++channel) {
        float* channelData = buffer.getWritePointer(channel);
        for (int sample = 0; sample < numSamples; ++sample) {
            float s = channelData[sample] * inputGainSmoother[channel].getNextValue();
            currentBlockInPeak = std::max(currentBlockInPeak, std::abs(s));

            // ★ Dry信号をリングバッファへ格納
            dryDelayBuffer[channel][dryDelayWritePos[channel]] = s;
            dryDelayWritePos[channel] = (dryDelayWritePos[channel] + 1) % 32768;

            channelData[sample] = s;
        }
    }

    // [Phase 2 & 3] Oversampling & DSP
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::AudioBlock<float> upsampledBlock;
    if (currentOsMode > 0) upsampledBlock = oversamplers[currentOsMode]->processSamplesUp(block);
    else upsampledBlock = block;

    const int numSamplesHigh = upsampledBlock.getNumSamples();
    for (int channel = 0; channel < upsampledBlock.getNumChannels(); ++channel) {
        float* channelData = upsampledBlock.getChannelPointer(channel);
        for (int sample = 0; sample < numSamplesHigh; ++sample) {
            float s = channelData[sample];

            s = inTransEngines[channel][currentInTransIdx]->processSample(s);
            s = preampEngines[channel][currentPreampIdx]->processSample(s, driveSmoother[channel].getNextValue(), charSmoother[channel].getNextValue(), asymSmoother[channel].getNextValue(), ageSmoother[channel].getNextValue());
            s = outTransEngines[channel][currentOutTransIdx]->processSample(s, colorSmoother[channel].getNextValue(), airSmoother[channel].getNextValue(), ageSmoother[channel].getNextValue());

            channelData[sample] = s;
        }
    }

    if (currentOsMode > 0) oversamplers[currentOsMode]->processSamplesDown(block);

    // ★ [Delay Compensation 計算] オーバーサンプリング遅延 + 2次ADAA遅延(高サンプルレート基準で1.0サンプル)
    float osLatency = (currentOsMode > 0) ? oversamplers[currentOsMode]->getLatencyInSamples() : 0.0f;
    float adaaLatency = 1.0f / (1 << currentOsMode);
    float totalDelay = osLatency + adaaLatency;

    // [Phase 4] Mix, Wet Recording & Output Gain
    for (int channel = 0; channel < totalNumInputChannels; ++channel) {
        float* channelData = buffer.getWritePointer(channel);

        // 当該ブロックの先頭読み取り位置 (書き込み位置からブロックサイズと遅延分を遡る)
        float readPosBase = static_cast<float>(dryDelayWritePos[channel] - numSamples) - totalDelay;

        for (int sample = 0; sample < numSamples; ++sample) {
            float wetSignal = channelData[sample];

            // ★ 端数遅延補間 (Linear Interpolation)
            float readPosF = readPosBase + static_cast<float>(sample);
            while (readPosF < 0.0f) readPosF += 32768.0f;
            while (readPosF >= 32768.0f) readPosF -= 32768.0f;

            int idx1 = static_cast<int>(readPosF);
            int idx2 = (idx1 + 1) % 32768;
            float frac = readPosF - static_cast<float>(idx1);

            // 遅延補正された完全なDry信号
            float delayedDry = dryDelayBuffer[channel][idx1] * (1.0f - frac) + dryDelayBuffer[channel][idx2] * frac;

            // AutoLevelへ信号を供給 (Dry/Wetのアライメントが保証される)
            autoLevels[channel].pushDrySample(delayedDry);

            float mixRatio = mixSmoother[channel].getNextValue();
            float mixedSignal = delayedDry + (wetSignal - delayedDry) * mixRatio;
            float finalSignal = isListenDry ? delayedDry : mixedSignal;

            autoLevels[channel].pushWetSample(mixedSignal);

            finalSignal *= outputGainSmoother[channel].getNextValue();
            channelData[sample] = finalSignal;

            currentBlockOutPeak = std::max(currentBlockOutPeak, std::abs(finalSignal));

            if (channel == 0) pushNextSampleIntoFifo(finalSignal);
        }
    }

    // ==============================================================================
    // ピークの保持と減衰計算
    // ==============================================================================
    float blockDecay = static_cast<float>(std::exp(-1.0 / (0.1 * currentSampleRate) * numSamples));
    inPeakState = std::max(currentBlockInPeak, inPeakState * blockDecay);
    outPeakState = std::max(currentBlockOutPeak, outPeakState * blockDecay);

    inputPeakDb.store(juce::Decibels::gainToDecibels(inPeakState, -60.0f));
    outputPeakDb.store(juce::Decibels::gainToDecibels(outPeakState, -60.0f));
}

const juce::String NeotoPreAudioProcessor::getName() const { return JucePlugin_Name; }
bool NeotoPreAudioProcessor::acceptsMidi() const { return false; }
bool NeotoPreAudioProcessor::producesMidi() const { return false; }
bool NeotoPreAudioProcessor::isMidiEffect() const { return false; }
double NeotoPreAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int NeotoPreAudioProcessor::getNumPrograms() { return 1; }
int NeotoPreAudioProcessor::getCurrentProgram() { return 0; }
void NeotoPreAudioProcessor::setCurrentProgram(int index) {}
const juce::String NeotoPreAudioProcessor::getProgramName(int index) { return {}; }
void NeotoPreAudioProcessor::changeProgramName(int index, const juce::String& newName) {}
bool NeotoPreAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* NeotoPreAudioProcessor::createEditor() { return new NeotoPreAudioProcessorEditor(*this); }

void NeotoPreAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void NeotoPreAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

void NeotoPreAudioProcessor::pushNextSampleIntoFifo(float sample) noexcept
{
    if (audioFifo.getFreeSpace() > 0) {
        int start1, block1, start2, block2;
        audioFifo.prepareToWrite(1, start1, block1, start2, block2);
        if (block1 > 0) audioFifoBuffer[(size_t)start1] = sample;
        if (block2 > 0) audioFifoBuffer[(size_t)start2] = sample;
        audioFifo.finishedWrite(1);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new NeotoPreAudioProcessor(); }