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
    driveParam = apvts.getRawParameterValue("drive");
    colorParam = apvts.getRawParameterValue("color");
    charParam = apvts.getRawParameterValue("character");
    asymParam = apvts.getRawParameterValue("asymmetry");
    airParam = apvts.getRawParameterValue("air");
    ageParam = apvts.getRawParameterValue("age");
    analysisTimeParam = apvts.getRawParameterValue("analysis_time");
    autoLevelTargetParam = apvts.getRawParameterValue("autolevel_target");

    oversampler = std::make_unique<juce::dsp::Oversampling<float>>(
        2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true
    );
}

NeotoPreAudioProcessor::~NeotoPreAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout NeotoPreAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

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

    // 解析時間コンボボックス用パラメーター
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ "analysis_time", 1 }, "Analysis Time",
        juce::StringArray{ "1 sec", "3 sec", "5 sec", "10 sec" }, 1)); // Default: 3 sec

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "autolevel_target", 1 }, "Target LUFS", -36.0f, -6.0f, -18.0f));

    return { params.begin(), params.end() };
}

void NeotoPreAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    for (auto& al : autoLevels) al.prepare(sampleRate);

    oversampler->reset();
    oversampler->initProcessing(samplesPerBlock);

    // 大きな変更点：AutoLevelが遅延を持たなくなったため、レイテンシーはOSの分のみ！
    setLatencySamples(static_cast<int>(oversampler->getLatencyInSamples()));

    double oversampledRate = sampleRate * oversampler->getOversamplingFactor();
    for (int i = 0; i < 2; ++i)
    {
        inputTransformers[i].prepare(oversampledRate);
        apiDrives[i].prepare(oversampledRate);
        outputTransformers[i].prepare(oversampledRate);

        driveSmoother[i].reset(oversampledRate, 0.02);
        colorSmoother[i].reset(oversampledRate, 0.02);
        charSmoother[i].reset(oversampledRate, 0.02);
        asymSmoother[i].reset(oversampledRate, 0.02);
        airSmoother[i].reset(oversampledRate, 0.02);
        ageSmoother[i].reset(oversampledRate, 0.02);

        driveSmoother[i].setCurrentAndTargetValue(driveParam->load());
        colorSmoother[i].setCurrentAndTargetValue(colorParam->load());
        charSmoother[i].setCurrentAndTargetValue(charParam->load());
        asymSmoother[i].setCurrentAndTargetValue(asymParam->load());
        airSmoother[i].setCurrentAndTargetValue(airParam->load());
        ageSmoother[i].setCurrentAndTargetValue(ageParam->load());
    }
}

void NeotoPreAudioProcessor::releaseResources() { oversampler->reset(); }

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

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // ==============================================================================
    // 0. AutoLevel 計算トリガーの監視 (UIのApplyボタンが押された瞬間のみ実行)
    // ==============================================================================
    if (triggerAutoLevel.exchange(false))
    {
        int timeSelection = static_cast<int>(analysisTimeParam->load());
        float seconds = 3.0f;
        if (timeSelection == 0) seconds = 1.0f;
        else if (timeSelection == 1) seconds = 3.0f;
        else if (timeSelection == 2) seconds = 5.0f;
        else if (timeSelection == 3) seconds = 10.0f;

        float target = autoLevelTargetParam->load();

        // ステレオ両チャンネルのRMSを計測し、大きい方を基準とする（センター定位の崩れ防止）
        float rmsL = autoLevels[0].calculateRMS(seconds);
        float rmsR = totalNumInputChannels > 1 ? autoLevels[1].calculateRMS(seconds) : rmsL;
        float maxRms = std::max(rmsL, rmsR);

        // 無音時（-80dB以下）にノイズを爆増させないためのセーフティ
        if (maxRms > 0.0001f)
        {
            float currentDb = juce::Decibels::gainToDecibels(maxRms);
            float diffDb = target - currentDb;
            float newGain = juce::Decibels::decibelsToGain(diffDb);

            // ステレオリンクで同じゲインを適用
            autoLevels[0].setTargetGain(newGain);
            if (totalNumInputChannels > 1) autoLevels[1].setTargetGain(newGain);
        }
    }

    const float rawDrive = driveParam->load();
    const float rawColor = colorParam->load();
    const float rawChar = charParam->load();
    const float rawAsym = asymParam->load();
    const float rawAir = airParam->load();
    const float rawAge = ageParam->load();

    for (int i = 0; i < 2; ++i) {
        driveSmoother[i].setTargetValue(rawDrive);
        colorSmoother[i].setTargetValue(rawColor);
        charSmoother[i].setTargetValue(rawChar);
        asymSmoother[i].setTargetValue(rawAsym);
        airSmoother[i].setTargetValue(rawAir);
        ageSmoother[i].setTargetValue(rawAge);
    }

    // フェーズ1: ヒストリーバッファへの記録 (※波形は遅延させない)
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        float* channelData = buffer.getWritePointer(channel);
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            autoLevels[channel].pushSample(channelData[sample]);
        }
    }

    // フェーズ2: Upsample
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::AudioBlock<float> upsampledBlock = oversampler->processSamplesUp(block);

    // フェーズ3: High-Rate DSP Processing
    const int numSamplesHigh = upsampledBlock.getNumSamples();
    const int numChannelsHigh = upsampledBlock.getNumChannels();

    for (int channel = 0; channel < numChannelsHigh; ++channel)
    {
        float* channelData = upsampledBlock.getChannelPointer(channel);
        for (int sample = 0; sample < numSamplesHigh; ++sample)
        {
            float curDrive = driveSmoother[channel].getNextValue();
            float curColor = colorSmoother[channel].getNextValue();
            float curChar = charSmoother[channel].getNextValue();
            float curAsym = asymSmoother[channel].getNextValue();
            float curAir = airSmoother[channel].getNextValue();
            float curAge = ageSmoother[channel].getNextValue();

            float s = channelData[sample];
            s = inputTransformers[channel].processSample(s);
            s = apiDrives[channel].processSample(s, curDrive, curChar, curAsym, curAge);
            s = outputTransformers[channel].processSample(s, curColor, curAir, curAge);
            channelData[sample] = s;
        }
    }

    // フェーズ4: Downsample
    oversampler->processSamplesDown(block);

    // フェーズ5: Post-Oversampling (AutoLevel Gain Application)
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        float* channelData = buffer.getWritePointer(channel);
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            channelData[sample] = autoLevels[channel].processApplication(channelData[sample]);
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