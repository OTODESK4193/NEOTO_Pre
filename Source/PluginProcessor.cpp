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
    driveParam = apvts.getRawParameterValue("drive");
    colorParam = apvts.getRawParameterValue("color");
    charParam = apvts.getRawParameterValue("character");
    asymParam = apvts.getRawParameterValue("asymmetry");
    airParam = apvts.getRawParameterValue("air");
    ageParam = apvts.getRawParameterValue("age");
    analysisTimeParam = apvts.getRawParameterValue("analysis_time");

    for (int i = 0; i < 3; ++i) {
        oversamplers[i] = std::make_unique<juce::dsp::Oversampling<float>>(
            2, i, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true);
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
        juce::StringArray{ "Off (1x)", "2x", "4x" }, 1));

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

    dryBuffer.setSize(getTotalNumInputChannels(), samplesPerBlock);

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
    for (int i = 0; i < 2; ++i) {
        inputTransformers[i].prepare(oversampledRate);
        apiDrives[i].prepare(oversampledRate);
        outputTransformers[i].prepare(oversampledRate);

        driveSmoother[i].reset(oversampledRate, 0.02);
        colorSmoother[i].reset(oversampledRate, 0.02);
        charSmoother[i].reset(oversampledRate, 0.02);
        asymSmoother[i].reset(oversampledRate, 0.02);
        airSmoother[i].reset(oversampledRate, 0.02);
        ageSmoother[i].reset(oversampledRate, 0.02);
    }

    int latency = currentOsMode > 0 ? static_cast<int>(oversamplers[currentOsMode]->getLatencyInSamples()) : 0;
    setLatencySamples(latency);
}

void NeotoPreAudioProcessor::releaseResources()
{
    for (auto& os : oversamplers) os->reset();
    dryBuffer.setSize(0, 0);
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
        for (int i = 0; i < 2; ++i) {
            inputTransformers[i].prepare(newRate);
            apiDrives[i].prepare(newRate);
            outputTransformers[i].prepare(newRate);
            driveSmoother[i].reset(newRate, 0.02); colorSmoother[i].reset(newRate, 0.02);
            charSmoother[i].reset(newRate, 0.02); asymSmoother[i].reset(newRate, 0.02);
            airSmoother[i].reset(newRate, 0.02); ageSmoother[i].reset(newRate, 0.02);
        }
        int latency = currentOsMode > 0 ? static_cast<int>(oversamplers[currentOsMode]->getLatencyInSamples()) : 0;
        setLatencySamples(latency);
    }

    // ==============================================================================
    // [Analyze] トリガーの処理 (Gain Matching 計算)
    // ==============================================================================
    if (triggerAnalyze.exchange(false))
    {
        int timeSelection = static_cast<int>(analysisTimeParam->load());
        float seconds = (timeSelection == 0) ? 1.0f : (timeSelection == 1) ? 3.0f : (timeSelection == 2) ? 5.0f : 10.0f;

        for (int ch = 0; ch < totalNumInputChannels; ++ch) autoLevels[ch].analyzeRMS(seconds);

        latestAnalysisResult.dryRmsL = autoLevels[0].getLatestDryRMS();
        latestAnalysisResult.wetRmsL = autoLevels[0].getLatestWetRMS();
        if (totalNumInputChannels > 1) {
            latestAnalysisResult.dryRmsR = autoLevels[1].getLatestDryRMS();
            latestAnalysisResult.wetRmsR = autoLevels[1].getLatestWetRMS();
        }
        else {
            latestAnalysisResult.dryRmsR = latestAnalysisResult.dryRmsL;
            latestAnalysisResult.wetRmsR = latestAnalysisResult.wetRmsL;
        }

        // ゲインマッチングの算出 (Output = Wet + OutputGain。 Dry = Output となる OutputGain を求める)
        float maxDryRms = std::max(latestAnalysisResult.dryRmsL, latestAnalysisResult.dryRmsR);
        float maxWetRms = std::max(latestAnalysisResult.wetRmsL, latestAnalysisResult.wetRmsR);

        if (maxWetRms > 0.0001f && maxDryRms > 0.0001f) {
            float dryDb = juce::Decibels::gainToDecibels(maxDryRms);
            float wetDb = juce::Decibels::gainToDecibels(maxWetRms);
            latestAnalysisResult.suggestedGainDb = dryDb - wetDb; // 完璧なゲインマッチング
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

    // [Phase 1] Input Gain & Dry Recording
    for (int channel = 0; channel < totalNumInputChannels; ++channel) {
        float* channelData = buffer.getWritePointer(channel);
        float* dryData = dryBuffer.getWritePointer(channel);
        for (int sample = 0; sample < numSamples; ++sample) {
            float s = channelData[sample] * inputGainSmoother[channel].getNextValue();
            autoLevels[channel].pushDrySample(s);
            channelData[sample] = s;
            dryData[sample] = s;
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
            s = inputTransformers[channel].processSample(s);
            s = apiDrives[channel].processSample(s, driveSmoother[channel].getNextValue(), charSmoother[channel].getNextValue(), asymSmoother[channel].getNextValue(), ageSmoother[channel].getNextValue());
            s = outputTransformers[channel].processSample(s, colorSmoother[channel].getNextValue(), airSmoother[channel].getNextValue(), ageSmoother[channel].getNextValue());
            channelData[sample] = s;
        }
    }

    if (currentOsMode > 0) oversamplers[currentOsMode]->processSamplesDown(block);

    // [Phase 4] Mix, Wet Recording & Output Gain
    for (int channel = 0; channel < totalNumInputChannels; ++channel) {
        float* channelData = buffer.getWritePointer(channel);
        const float* dryData = dryBuffer.getReadPointer(channel);

        for (int sample = 0; sample < numSamples; ++sample) {
            float wetSignal = channelData[sample];
            float drySignal = dryData[sample];
            float mixRatio = mixSmoother[channel].getNextValue();

            float mixedSignal = drySignal + (wetSignal - drySignal) * mixRatio;
            float finalSignal = isListenDry ? drySignal : mixedSignal;

            // AutoLevel適用前のWet状態を記録
            autoLevels[channel].pushWetSample(mixedSignal);

            // Output Gain適用 (AutoLevelクラスへの依存を完全排除)
            finalSignal *= outputGainSmoother[channel].getNextValue();
            channelData[sample] = finalSignal;
        }
    }
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

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new NeotoPreAudioProcessor(); }