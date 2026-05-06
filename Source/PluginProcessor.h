#pragma once
#include <JuceHeader.h>
#include <vector>
#include <memory>
#include <atomic>
#include <array>
#include "DSP/Core/AutoLevel.h"
#include "DSP/Algorithms/PreampModels.h"
#include "DSP/Transformers/TransformerModels.h"

struct AutoLevelAnalysisResult {
    float dryRmsL = 0.0f; float dryRmsR = 0.0f;
    float wetRmsL = 0.0f; float wetRmsR = 0.0f;
    float suggestedGainDb = 0.0f;
};

class NeotoPreAudioProcessor : public juce::AudioProcessor
{
public:
    NeotoPreAudioProcessor();
    ~NeotoPreAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState apvts;

    // ==============================================================================
    // GUIとDSPスレッドを切り離すための解析システム
    // ==============================================================================
    std::atomic<bool> triggerAnalyze{ false };
    AutoLevelAnalysisResult latestAnalysisResult;
    std::atomic<bool> hasNewAnalysisResult{ false };

    // RMS解析をAudioThread外で実行するためのパブリックメソッド
    void executeAnalyzer(float seconds);

    std::atomic<float> inputPeakDb{ -60.0f };
    std::atomic<float> outputPeakDb{ -60.0f };
    std::atomic<float> harmonicLevels[7];

    juce::AbstractFifo audioFifo{ 8192 };
    std::array<float, 8192> audioFifoBuffer;
    void pushNextSampleIntoFifo(float sample) noexcept;

private:
    std::array<std::unique_ptr<juce::dsp::Oversampling<float>>, 4> oversamplers;
    int currentOsMode = -1;
    double currentSampleRate = 44100.0;
    std::array<AutoLevel, 2> autoLevels;

    float inPeakState = 0.0f;
    float outPeakState = 0.0f;

    std::unique_ptr<IInputTransformerEngine> inTransEngines[2][5];
    std::unique_ptr<IPreampEngine> preampEngines[2][6];
    std::unique_ptr<IOutputTransformerEngine> outTransEngines[2][5];

    std::atomic<float>* inputGainParam = nullptr;
    std::atomic<float>* outputGainParam = nullptr;
    std::atomic<float>* mixParam = nullptr;
    std::atomic<float>* listenModeParam = nullptr;
    std::atomic<float>* osModeParam = nullptr;

    std::atomic<float>* inTransParam = nullptr;
    std::atomic<float>* preampModelParam = nullptr;
    std::atomic<float>* outTransParam = nullptr;

    std::atomic<float>* driveParam = nullptr;
    std::atomic<float>* colorParam = nullptr;
    std::atomic<float>* charParam = nullptr;
    std::atomic<float>* asymParam = nullptr;
    std::atomic<float>* airParam = nullptr;
    std::atomic<float>* ageParam = nullptr;
    std::atomic<float>* analysisTimeParam = nullptr;

    std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>, 2> inputGainSmoother;
    std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>, 2> outputGainSmoother;
    std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>, 2> mixSmoother;

    std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>, 2> driveSmoother;
    std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>, 2> colorSmoother;
    std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>, 2> charSmoother;
    std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>, 2> asymSmoother;
    std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>, 2> airSmoother;
    std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>, 2> ageSmoother;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NeotoPreAudioProcessor)
};