#pragma once
#include <atomic>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "AutoTuneEngine.h"

class AutoWaveAudioProcessor : public juce::AudioProcessor
{
public:
    AutoWaveAudioProcessor();
    ~AutoWaveAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "AUTO WAVE"; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // Lecturas para la interfaz (nota detectada, corrección aplicada, nivel)
    std::atomic<float> levelMeter { 0.0f }, detectedMidi { 0.0f }, correctionSemis { 0.0f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    using Delay = juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear>;

    AutoTuneEngine engine;
    Delay dblA, dblB, dlyL, dlyR;
    juce::Reverb reverb;

    double sr = 44100.0;
    float  fbLp = 0.0f, phA = 0.0f, phB = 0.37f;
    int    currentProgram = 0, lastInputType = -1;

    std::atomic<float> *pKey, *pScale, *pMode, *pInput, *pRetune, *pFlex, *pHumanize, *pTranspose,
                       *pVibAmt, *pVibRate, *pMix, *pLow, *pDoubler, *pDrive,
                       *pDlyTime, *pDlyFb, *pDlyMix, *pRevSize, *pRevMix, *pOut;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AutoWaveAudioProcessor)
};
