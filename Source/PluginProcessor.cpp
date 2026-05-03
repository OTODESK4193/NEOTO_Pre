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

    // 追加パラメーターの取得
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

    // オーバーサンプリングの初期化
    for (int i = 0; i < 3; ++i) {
        oversamplers[i] = std::make_unique<juce::dsp::Oversampling<float>>(
            2, i, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true);
    }

    // ==============================================================================
    // 全モデルのインスタンス生成 (メモリ上に確保)
    // ==============================================================================
    for (int ch = 0; ch < 2; ++ch) {
        // Input Transformers
        inTransEngines[ch][0] = std::make_unique<InputTransformer_None>();
        inTransEngines[ch][1] = std::make_unique<InputTransformer_Nickel>();
        inTransEngines[ch][2] = std::make_unique<InputTransformer_Steel>();
        inTransEngines[ch][3] = std::make_unique<InputTransformer_Iron>();
        inTransEngines[ch][4] = std::make_unique<InputTransformer_Amorphous>();

        // Preamps
        preampEngines[ch][0] = std::make_unique<Preamp_API>();
        preampEngines[ch][1] = std::make_unique<Preamp_Neve>();
        preampEngines[ch][2] = std::make_unique<Preamp_Tube>();
        preampEngines[ch][3] = std::make_unique<Preamp_SSL>();
        preampEngines[ch][4] = std::make_unique<Preamp_Modern1>();
        preampEngines[ch][5] = std::make_unique<Preamp_Modern2>();

        // Output Transformers
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
        juce::StringArray{ "Off (1x)", "2x", "4x" }, 1));

    // ==============================================================================
    // 新規追加パラメーター（APVTS登録）※GUIの文字列と完全に一致させます
    // ==============================================================================
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

    // 全てのインスタンスの prepare() を呼び出してスタンバイさせる
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

    // ==============================================================================
    // [Analyze] トリガーの処理 (Gain Matching)
    // ==============================================================================
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

    // 現在選ばれているモデルのインデックスを取得
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

    // [Phase 2 & 3] Oversampling & DSP (Polymorphic Processing)
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::AudioBlock<float> upsampledBlock;
    if (currentOsMode > 0) upsampledBlock = oversamplers[currentOsMode]->processSamplesUp(block);
    else upsampledBlock = block;

    const int numSamplesHigh = upsampledBlock.getNumSamples();
    for (int channel = 0; channel < upsampledBlock.getNumChannels(); ++channel) {
        float* channelData = upsampledBlock.getChannelPointer(channel);
        for (int sample = 0; sample < numSamplesHigh; ++sample) {
            float s = channelData[sample];

            // ==============================================================================
            // 切り替えロジック：選ばれたインスタンスの processSample() を叩くだけ！
            // ==============================================================================
            s = inTransEngines[channel][currentInTransIdx]->processSample(s);
            s = preampEngines[channel][currentPreampIdx]->processSample(s, driveSmoother[channel].getNextValue(), charSmoother[channel].getNextValue(), asymSmoother[channel].getNextValue(), ageSmoother[channel].getNextValue());
            s = outTransEngines[channel][currentOutTransIdx]->processSample(s, colorSmoother[channel].getNextValue(), airSmoother[channel].getNextValue(), ageSmoother[channel].getNextValue());

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

            autoLevels[channel].pushWetSample(mixedSignal);

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