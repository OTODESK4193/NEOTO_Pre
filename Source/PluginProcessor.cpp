#pragma execution_character_set("utf-8")
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm> // std::clamp用

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
        inTransEngines[ch][5] = std::make_unique<InputTransformer_Carnhill>();
        inTransEngines[ch][6] = std::make_unique<InputTransformer_Cinemag>();

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
        outTransEngines[ch][5] = std::make_unique<OutputTransformer_Carnhill>();
        outTransEngines[ch][6] = std::make_unique<OutputTransformer_Cinemag>();
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
        juce::StringArray{ "None", "Nickel", "Steel", "Iron", "Amorphous", "Carnhill", "Cinemag" }, 1));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ "preamp_model", 1 }, "Preamp Model",
        juce::StringArray{ "API Style", "Neve Style", "Vintage Tube", "SSL Modern", "TG2 (Modern)", "B173 (Modern)" }, 0));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ "out_trans_type", 1 }, "Output Transformer",
        juce::StringArray{ "None", "Nickel", "Steel", "Iron", "Amorphous", "Carnhill", "Cinemag" }, 1));

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

    for (auto& al : autoLevels) al.prepare(sampleRate);
    for (auto& os : oversamplers) os->initProcessing(samplesPerBlock);

    // Listenモード用: InputGain適用後の純粋なドライ信号を保持
    listenDryBuffer.setSize(2, samplesPerBlock, false, true);

    for (int i = 0; i < 2; ++i) {
        inputGainSmoother[i].reset(sampleRate, 0.02);
        outputGainSmoother[i].reset(sampleRate, 0.15);
        mixSmoother[i].reset(sampleRate, 0.02);
        inputGainSmoother[i].setCurrentAndTargetValue(juce::Decibels::decibelsToGain(inputGainParam->load()));
        outputGainSmoother[i].setCurrentAndTargetValue(juce::Decibels::decibelsToGain(outputGainParam->load()));
        mixSmoother[i].setCurrentAndTargetValue(mixParam->load() / 100.0f);
    }

    double oversampledRate = sampleRate * (1 << currentOsMode);

    for (int ch = 0; ch < 2; ++ch) {
        for (int i = 0; i < 7; ++i) inTransEngines[ch][i]->prepare(oversampledRate);
        for (int i = 0; i < 6; ++i) preampEngines[ch][i]->prepare(oversampledRate);
        for (int i = 0; i < 7; ++i) outTransEngines[ch][i]->prepare(oversampledRate);

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

// ==============================================================================
// 解析ステートマシン: 開始
// ==============================================================================
void NeotoPreAudioProcessor::startAnalysis(float seconds)
{
    // 進行中の解析をキャンセル
    analysisPhaseState.store(0);

    analysisSeconds = seconds;
    int numSamples = static_cast<int>(currentSampleRate * seconds);

    for (int ch = 0; ch < 2; ++ch) {
        autoLevels[ch].startRecording(numSamples);
    }

    hasSweetSpotResult.store(false);
    hasNewAnalysisResult.store(false);

    // SweetSpotフェーズ開始
    analysisPhaseState.store(1);
}

// ==============================================================================
// 解析ステートマシン: フェーズ遷移確認（Message Threadから呼ばれる）
// ==============================================================================
void NeotoPreAudioProcessor::updateAnalysisState()
{
    int phase = analysisPhaseState.load();

    // ---- Phase 1: SweetSpot 完了判定 ----
    if (phase == 1 && autoLevels[0].isComplete())
    {
        // K-Weighted 入力レベルを計算
        float dryEnergyL = autoLevels[0].getDryMeanSquare();
        float dryEnergyR = getTotalNumInputChannels() > 1 ? autoLevels[1].getDryMeanSquare() : dryEnergyL;
        float totalDryEnergy = dryEnergyL + dryEnergyR;

        float measuredDb = -100.0f;
        if (totalDryEnergy > 1e-10f)
            measuredDb = 10.0f * std::log10(totalDryEnergy) - 0.691f;

        // InputGainを -18 dBFS SweetSpotに調整
        float targetDb = -18.0f;
        float suggestedInputGain = targetDb - measuredDb;
        suggestedInputGain = std::clamp(suggestedInputGain, -24.0f, 24.0f);

        // SweetSpot結果を保存
        sweetSpotMeasuredDb = measuredDb;
        sweetSpotInputGainDb = suggestedInputGain;
        hasSweetSpotResult.store(true);

        // InputGainパラメータを適用
        auto* param = apvts.getParameter("input_gain");
        param->setValueNotifyingHost(param->convertTo0to1(suggestedInputGain));

        // AutoLevelフェーズを開始
        int numSamples = static_cast<int>(currentSampleRate * analysisSeconds);
        for (int ch = 0; ch < getTotalNumInputChannels(); ++ch) {
            autoLevels[ch].startRecording(numSamples);
        }
        analysisPhaseState.store(2);
    }
    // ---- Phase 2: AutoLevel 完了判定 ----
    else if (phase == 2 && autoLevels[0].isComplete())
    {
        float dryEnergyL = autoLevels[0].getDryMeanSquare();
        float wetEnergyL = autoLevels[0].getWetMeanSquare();
        float dryEnergyR = getTotalNumInputChannels() > 1 ? autoLevels[1].getDryMeanSquare() : dryEnergyL;
        float wetEnergyR = getTotalNumInputChannels() > 1 ? autoLevels[1].getWetMeanSquare() : wetEnergyL;

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
        analysisPhaseState.store(3); // Done
    }
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
            for (int i = 0; i < 7; ++i) inTransEngines[ch][i]->prepare(newRate);
            for (int i = 0; i < 6; ++i) preampEngines[ch][i]->prepare(newRate);
            for (int i = 0; i < 7; ++i) outTransEngines[ch][i]->prepare(newRate);

            driveSmoother[ch].reset(newRate, 0.02);
            outputGainSmoother[ch].reset(newRate, 0.15);
            colorSmoother[ch].reset(newRate, 0.02);
            charSmoother[ch].reset(newRate, 0.02);
            asymSmoother[ch].reset(newRate, 0.02);
            airSmoother[ch].reset(newRate, 0.02);
            ageSmoother[ch].reset(newRate, 0.02);
        }
        int latency = currentOsMode > 0 ? static_cast<int>(oversamplers[currentOsMode]->getLatencyInSamples()) : 0;
        setLatencySamples(latency);
    }

    float targetInGain = juce::Decibels::decibelsToGain(inputGainParam->load());
    float targetOutGain = juce::Decibels::decibelsToGain(outputGainParam->load());
    float targetMix = mixParam->load() / 100.0f;
    bool isListenDry = listenModeParam->load() > 0.5f;

    int currentInTransIdx = std::clamp(static_cast<int>(inTransParam->load()), 0, 6);
    int currentPreampIdx = std::clamp(static_cast<int>(preampModelParam->load()), 0, 5);
    int currentOutTransIdx = std::clamp(static_cast<int>(outTransParam->load()), 0, 6);

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

    int currentPhase = analysisPhaseState.load(std::memory_order_relaxed);

    for (int channel = 0; channel < totalNumInputChannels; ++channel) {
        float* channelData = buffer.getWritePointer(channel);
        for (int sample = 0; sample < numSamples; ++sample) {
            float rawInput = channelData[sample];
            float s = rawInput * inputGainSmoother[channel].getNextValue();
            currentBlockInPeak = std::max(currentBlockInPeak, std::abs(s));

            // フェーズ別のサンプル記録
            if (currentPhase == 1) {
                // SweetSpot: 原音（InputGain前）を測定
                autoLevels[channel].pushDrySample(rawInput);
            } else if (currentPhase == 2) {
                // AutoLevel: InputGain適用後の信号を測定
                autoLevels[channel].pushDrySample(s);
            }

            channelData[sample] = s;

            // Listenモード用: InputGainだけを通った純粋なドライ信号を保存
            listenDryBuffer.setSample(channel, sample, s);
        }
    }

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::AudioBlock<float> upsampledBlock;
    if (currentOsMode > 0) upsampledBlock = oversamplers[currentOsMode]->processSamplesUp(block);
    else upsampledBlock = block;

    const int numSamplesHigh = upsampledBlock.getNumSamples();

    for (int channel = 0; channel < upsampledBlock.getNumChannels(); ++channel) {
        float* channelData = upsampledBlock.getChannelPointer(channel);

        for (int sample = 0; sample < numSamplesHigh; ++sample) {

            float curDrive = driveSmoother[channel].getNextValue();
            float curChar = charSmoother[channel].getNextValue();
            float curAsym = asymSmoother[channel].getNextValue();
            float curColor = colorSmoother[channel].getNextValue();
            float curAir = airSmoother[channel].getNextValue();
            float curAge = ageSmoother[channel].getNextValue();

            float wetSignal = channelData[sample];
            float drySignal = wetSignal;

            wetSignal = inTransEngines[channel][currentInTransIdx]->processSample(wetSignal);
            drySignal = inTransEngines[channel][currentInTransIdx]->processDrySample(drySignal);

            wetSignal = preampEngines[channel][currentPreampIdx]->processSample(
                wetSignal, curDrive, curChar, curAsym, curAge, curColor);

            drySignal = preampEngines[channel][currentPreampIdx]->processDrySample(
                drySignal, curDrive, curChar, curAsym, curAge, curColor);

            wetSignal = outTransEngines[channel][currentOutTransIdx]->processSample(
                wetSignal, curColor, curAir, curAge);

            drySignal = outTransEngines[channel][currentOutTransIdx]->processDrySample(
                drySignal, curAir, curAge);

            float mixRatio = mixSmoother[channel].getNextValue();
            float mixedSignal = drySignal + (wetSignal - drySignal) * mixRatio;

            // ★ Listenバイパスは最終出力ループで行う（解析用Wet信号を正しく計測するため）
            channelData[sample] = mixedSignal;
        }
    }

    if (currentOsMode > 0) oversamplers[currentOsMode]->processSamplesDown(block);

    for (int channel = 0; channel < totalNumInputChannels; ++channel) {
        float* channelData = buffer.getWritePointer(channel);

        for (int sample = 0; sample < numSamples; ++sample) {
            float finalSignal = channelData[sample] * outputGainSmoother[channel].getNextValue();

            // AutoLevelフェーズ: 処理済み信号（OutputGain適用後）を記録
            if (currentPhase == 2) {
                autoLevels[channel].pushWetSample(finalSignal);
            }

            // ★ Listenモード: InputGainだけを通った完全ドライ信号を出力
            float outputSignal = isListenDry ? listenDryBuffer.getSample(channel, sample) : finalSignal;
            channelData[sample] = outputSignal;
            currentBlockOutPeak = std::max(currentBlockOutPeak, std::abs(outputSignal));

            if (channel == 0) pushNextSampleIntoFifo(outputSignal);
        }
    }

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