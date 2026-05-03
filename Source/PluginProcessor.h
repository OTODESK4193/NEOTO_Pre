#pragma once

#include <JuceHeader.h>
#include <vector>
#include <memory>
#include <atomic>
#include <array>
#include "DSP/Core/AutoLevel.h"
#include "DSP/Algorithms/ApiStyleDrive.h"
#include "DSP/Transformers/SteelTransformer.h"
#include "DSP/Transformers/NickelTransformer.h"

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

    // UIから発火させるApplyトリガー
    std::atomic<bool> triggerAutoLevel{ false };

private:
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;

    std::array<AutoLevel, 2> autoLevels;
    std::array<NickelTransformer, 2> inputTransformers;
    std::array<ApiStyleDrive, 2> apiDrives;
    std::array<SteelTransformer, 2> outputTransformers;

    std::atomic<float>* driveParam = nullptr;
    std::atomic<float>* colorParam = nullptr;
    std::atomic<float>* charParam = nullptr;
    std::atomic<float>* asymParam = nullptr;
    std::atomic<float>* airParam = nullptr;
    std::atomic<float>* ageParam = nullptr;
    std::atomic<float>* analysisTimeParam = nullptr; // 追加
    std::atomic<float>* autoLevelTargetParam = nullptr;

    std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>, 2> driveSmoother;
    std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>, 2> colorSmoother;
    std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>, 2> charSmoother;
    std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>, 2> asymSmoother;
    std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>, 2> airSmoother;
    std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>, 2> ageSmoother;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NeotoPreAudioProcessor)
};