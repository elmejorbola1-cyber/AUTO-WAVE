#pragma once
#include "PluginProcessor.h"
#include "AutoWaveLookAndFeel.h"

class AutoWaveAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit AutoWaveAudioProcessorEditor (AutoWaveAudioProcessor&);
    ~AutoWaveAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAtt  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    struct Knob
    {
        juce::Slider slider;
        std::unique_ptr<SliderAtt> att;
        juce::String text;
        juce::Rectangle<int> plate;
    };

    struct Section
    {
        juce::String title;
        int row, startCol, count, first;
        juce::Rectangle<int> bounds;
    };

    void timerCallback() override;
    void addKnob (const juce::String& id, const juce::String& text);
    void addChoice (juce::ComboBox& box, const juce::String& id, std::unique_ptr<ComboAtt>& att);
    void drawWave (juce::Graphics& g, juce::Rectangle<float> area);
    static void drawPlate (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& text,
                           juce::Colour fill, float fontSize);

    static constexpr int headerH = 130;

    AutoWaveLookAndFeel laf;   // debe declararse primero: se destruye el último
    AutoWaveAudioProcessor& proc;
    double wavePhase = 0.0;

    juce::ComboBox presetBox, keyBox, scaleBox, inputBox, modeBox;
    std::unique_ptr<ComboAtt> keyAtt, scaleAtt, inputAtt, modeAtt;
    std::vector<std::pair<juce::String, juce::ComboBox*>> captions;

    std::vector<std::unique_ptr<Knob>> knobs;
    std::vector<Section> sections;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AutoWaveAudioProcessorEditor)
};
