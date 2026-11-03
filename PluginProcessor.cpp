#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <array>
#include <vector>

namespace
{
    const juce::StringArray keyNames   { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    const juce::StringArray scaleNames { "Chromatic", "Major", "Minor", "Harmonic Minor",
                                         "Minor Pentatonic", "Major Pentatonic", "Blues" };
    const juce::StringArray modeNames  { "Live", "Studio" };
    const juce::StringArray inputNames { "Soprano", "Alto / Tenor", "Low Male", "Wide / Instrument" };

    const std::array<std::vector<int>, 7> scaleNotes {{
        { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 },
        { 0, 2, 4, 5, 7, 9, 11 },
        { 0, 2, 3, 5, 7, 8, 10 },
        { 0, 2, 3, 5, 7, 8, 11 },
        { 0, 3, 5, 7, 10 },
        { 0, 2, 4, 7, 9 },
        { 0, 3, 5, 6, 7, 10 }
    }};

    unsigned maskFor (int scaleIdx)
    {
        unsigned m = 0;
        for (int n : scaleNotes[(size_t) juce::jlimit (0, 6, scaleIdx)]) m |= (1u << n);
        return m;
    }

    void rangeFor (int type, float& lo, float& hi)
    {
        switch (type)
        {
            case 0:  lo = 180.0f; hi = 1000.0f; break;   // Soprano
            case 1:  lo = 80.0f;  hi = 700.0f;  break;   // Alto / Tenor
            case 2:  lo = 60.0f;  hi = 350.0f;  break;   // Low Male
            default: lo = 60.0f;  hi = 1000.0f; break;   // Wide / Instrument
        }
    }

    // Presets "inspirados en el estilo" de cada artista (aproximaciones propias, no ajustes oficiales).
    // No tocan Key / Scale / Voice Type / Mode: ponlos según tu canción y tu voz.
    struct Preset
    {
        const char* name;
        float retune, flex, humanize, low, doubler, drive, dTime, dFb, dMix, rSize, rMix, vibAmt, vibRate;
    };

    const Preset presets[] = {
        { "Zeba (Signature)",   2.f,  0.f,  0.f, 25.f, 40.f, 22.f, 340.f, 36.f, 22.f, 80.f, 26.f, 10.f, 5.5f },
        { "Travis Scott",       0.f,  0.f,  0.f, 30.f, 40.f, 30.f, 330.f, 38.f, 24.f, 75.f, 25.f,  0.f, 5.5f },
        { "Jhay Weeler",        6.f,  0.f,  0.f, 10.f, 30.f, 15.f, 300.f, 30.f, 14.f, 50.f, 16.f,  0.f, 5.5f },
        { "Bad Bunny",         22.f, 10.f, 25.f,  0.f, 15.f, 12.f, 250.f, 20.f,  8.f, 35.f, 12.f,  8.f, 5.5f },
        { "Suei",               3.f,  0.f,  0.f, 15.f, 45.f, 20.f, 320.f, 35.f, 20.f, 70.f, 24.f,  0.f, 5.5f },
        { "Feid",              12.f,  0.f, 10.f,  0.f, 25.f,  8.f, 280.f, 25.f, 12.f, 55.f, 18.f,  6.f, 5.5f },
        { "Skaiy",              0.f,  0.f,  0.f, 20.f, 35.f, 25.f, 360.f, 40.f, 22.f, 65.f, 22.f,  0.f, 5.5f },
        { "Lil Juanse",         8.f,  0.f,  0.f,  0.f, 20.f, 10.f, 260.f, 25.f, 12.f, 45.f, 15.f,  0.f, 5.5f },
        { "Natural Correction",35.f, 20.f, 60.f,  0.f,  0.f,  0.f, 300.f, 30.f,  0.f, 40.f,  8.f,  0.f, 5.5f },
        { "Hard Tune Clean",    0.f,  0.f,  0.f,  0.f,  0.f,  0.f, 300.f, 30.f,  0.f, 40.f,  5.f,  0.f, 5.5f }
    };
    const int numPresets = (int) (sizeof (presets) / sizeof (presets[0]));
}

juce::AudioProcessorValueTreeState::ParameterLayout AutoWaveAudioProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    auto f = [&p] (const char* id, const char* name, float lo, float hi, float def,
                   const String& unit, int decimals, float skew = 1.0f)
    {
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name,
                        NormalisableRange<float> (lo, hi, 0.01f, skew), def,
                        AudioParameterFloatAttributes()
                            .withStringFromValueFunction ([unit, decimals] (float v, int)
                                                          { return String (v, decimals) + unit; })));
    };

    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "key", 1 },       "Key",        keyNames,   0));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "scale", 1 },     "Scale",      scaleNames, 0));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "inputType", 1 }, "Voice Type", inputNames, 1));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "mode", 1 },      "Mode",       modeNames,  0));

    f ("retune",     "Retune Speed", 0.f,   400.f, 20.f,  " ms",    0, 0.4f);
    f ("flex",       "Flex-Tune",    0.f,   50.f,  0.f,   " cents", 0);
    f ("humanize",   "Humanize",     0.f,   100.f, 0.f,   " %",     0);
    f ("transpose",  "Transpose",    -12.f, 12.f,  0.f,   " st",    1);
    f ("vibAmt",     "Vibrato",      0.f,   100.f, 0.f,   " %",     0);
    f ("vibRate",    "Vib Rate",     2.f,   9.f,   5.5f,  " Hz",    1);
    f ("mix",        "Mix",          0.f,   100.f, 100.f, " %",     0);
    f ("output",     "Output",       -24.f, 12.f,  0.f,   " dB",    1);
    f ("lowLayer",   "Low Layer",    0.f,   100.f, 0.f,   " %",     0);
    f ("doubler",    "Doubler",      0.f,   100.f, 0.f,   " %",     0);
    f ("drive",      "Drive",        0.f,   100.f, 0.f,   " %",     0);
    f ("delayTime",  "Delay Time",   60.f,  800.f, 300.f, " ms",    0);
    f ("delayFb",    "Delay FB",     0.f,   85.f,  35.f,  " %",     0);
    f ("delayMix",   "Delay Mix",    0.f,   100.f, 0.f,   " %",     0);
    f ("reverbSize", "Reverb Size",  0.f,   100.f, 50.f,  " %",     0);
    f ("reverbMix",  "Reverb Mix",   0.f,   100.f, 0.f,   " %",     0);

    return { p.begin(), p.end() };
}

AutoWaveAudioProcessor::AutoWaveAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    auto g = [this] (const char* id) { return apvts.getRawParameterValue (id); };
    pKey = g ("key"); pScale = g ("scale"); pMode = g ("mode"); pInput = g ("inputType");
    pRetune = g ("retune"); pFlex = g ("flex"); pHumanize = g ("humanize"); pTranspose = g ("transpose");
    pVibAmt = g ("vibAmt"); pVibRate = g ("vibRate"); pMix = g ("mix"); pOut = g ("output");
    pLow = g ("lowLayer"); pDoubler = g ("doubler"); pDrive = g ("drive");
    pDlyTime = g ("delayTime"); pDlyFb = g ("delayFb"); pDlyMix = g ("delayMix");
    pRevSize = g ("reverbSize"); pRevMix = g ("reverbMix");
}

bool AutoWaveAudioProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    if (l.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()) return false;
    return l.getMainInputChannelSet() == juce::AudioChannelSet::mono()
        || l.getMainInputChannelSet() == juce::AudioChannelSet::stereo();
}

void AutoWaveAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    engine.prepare (sr);
    lastInputType = -1;

    juce::dsp::ProcessSpec spec { sr, (juce::uint32) samplesPerBlock, 1 };
    for (auto* d : { &dblA, &dblB })
    {
        d->setMaximumDelayInSamples ((int) (0.06 * sr));
        d->prepare (spec);
    }
    for (auto* d : { &dlyL, &dlyR })
    {
        d->setMaximumDelayInSamples ((int) (1.2 * sr));
        d->prepare (spec);
    }

    reverb.setSampleRate (sr);
    reverb.reset();
    fbLp = 0.0f;

    setLatencySamples (0);   // Prioridad: uso en vivo (sin compensación de retardo en el host)
}

void AutoWaveAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int n = buffer.getNumSamples();
    const int inCh = getTotalNumInputChannels();
    if (buffer.getNumChannels() < 2) return;

    float* L = buffer.getWritePointer (0);
    float* R = buffer.getWritePointer (1);

    // --- ajustes del motor de afinación ---
    const int inputType = (int) pInput->load();
    if (inputType != lastInputType)
    {
        float lo, hi;
        rangeFor (inputType, lo, hi);
        engine.setInputRange (lo, hi);
        lastInputType = inputType;
    }

    TuneSettings ts;
    ts.retuneMs      = pRetune->load();
    ts.flexCents     = pFlex->load();
    ts.humanize      = pHumanize->load();
    ts.transpose     = pTranspose->load();
    ts.vibDepth      = pVibAmt->load() * 0.01f * 0.6f;      // hasta ±60 cents
    ts.vibRate       = pVibRate->load();
    ts.mix           = pMix->load() * 0.01f;
    ts.lowAmt        = pLow->load() * 0.01f;
    ts.key           = (int) pKey->load();
    ts.scaleMask     = maskFor ((int) pScale->load());
    ts.windowPeriods = ((int) pMode->load() == 0) ? 1 : 2;
    engine.update (ts);

    // --- efectos ---
    const float dblAmt = pDoubler->load() * 0.01f;
    const float drive = pDrive->load() * 0.01f;
    const float dlyTime = pDlyTime->load() * 0.001f * (float) sr;
    const float dlyFb = pDlyFb->load() * 0.01f, dlyMix = pDlyMix->load() * 0.01f;
    const float outGain = juce::Decibels::decibelsToGain (pOut->load());
    const float satGain = 1.0f + drive * 8.0f, satComp = 1.0f / std::sqrt (satGain);

    float peak = 0.0f;

    for (int i = 0; i < n; ++i)
    {
        const float in = inCh > 1 ? 0.5f * (L[i] + R[i]) : L[i];

        float v = engine.process (in);

        // saturación
        v = std::tanh (satGain * v) * satComp;

        // doubler estéreo
        const float dA = (0.018f + 0.0035f * std::sin (juce::MathConstants<float>::twoPi * phA)) * (float) sr;
        const float dB = (0.026f + 0.0045f * std::sin (juce::MathConstants<float>::twoPi * phB)) * (float) sr;
        phA += 0.45f / (float) sr; if (phA >= 1.0f) phA -= 1.0f;
        phB += 0.31f / (float) sr; if (phB >= 1.0f) phB -= 1.0f;

        const float aOut = dblA.popSample (0, dA);
        const float bOut = dblB.popSample (0, dB);
        dblA.pushSample (0, v);
        dblB.pushSample (0, v);

        float outL = v + dblAmt * 0.7f * aOut;
        float outR = v + dblAmt * 0.7f * bOut;

        // delay ping-pong
        const float dl = dlyL.popSample (0, dlyTime);
        const float dr = dlyR.popSample (0, dlyTime);
        fbLp += 0.35f * (dr - fbLp);
        dlyL.pushSample (0, std::tanh (v + dlyFb * fbLp));
        dlyR.pushSample (0, dl);
        outL += dlyMix * dl;
        outR += dlyMix * dr;

        L[i] = outL * outGain;
        R[i] = outR * outGain;
    }

    // reverb
    juce::Reverb::Parameters rp;
    rp.roomSize = pRevSize->load() * 0.01f;
    rp.damping = 0.45f;
    rp.wetLevel = pRevMix->load() * 0.01f * 0.33f;
    rp.dryLevel = 1.0f;
    rp.width = 1.0f;
    rp.freezeMode = 0.0f;
    reverb.setParameters (rp);
    reverb.processStereo (L, R, n);

    // datos para la interfaz
    for (int i = 0; i < n; ++i)
        peak = std::max (peak, std::max (std::abs (L[i]), std::abs (R[i])));
    levelMeter.store (std::max (peak, levelMeter.load() * 0.92f));
    detectedMidi.store (engine.getDetectedMidi());
    correctionSemis.store (engine.getCorrectionSemis());
}

int AutoWaveAudioProcessor::getNumPrograms() { return numPresets; }

const juce::String AutoWaveAudioProcessor::getProgramName (int index)
{
    return juce::isPositiveAndBelow (index, numPresets) ? presets[index].name : "";
}

void AutoWaveAudioProcessor::setCurrentProgram (int index)
{
    if (! juce::isPositiveAndBelow (index, numPresets)) return;
    currentProgram = index;
    const Preset& pr = presets[index];

    auto set = [this] (const char* id, float v)
    {
        if (auto* prm = apvts.getParameter (id))
            prm->setValueNotifyingHost (prm->convertTo0to1 (v));
    };

    set ("retune", pr.retune);     set ("flex", pr.flex);       set ("humanize", pr.humanize);
    set ("lowLayer", pr.low);      set ("doubler", pr.doubler); set ("drive", pr.drive);
    set ("delayTime", pr.dTime);   set ("delayFb", pr.dFb);     set ("delayMix", pr.dMix);
    set ("reverbSize", pr.rSize);  set ("reverbMix", pr.rMix);
    set ("vibAmt", pr.vibAmt);     set ("vibRate", pr.vibRate);
    set ("transpose", 0.0f);       set ("mix", 100.0f);         set ("output", 0.0f);
}

void AutoWaveAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void AutoWaveAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* AutoWaveAudioProcessor::createEditor()
{
    return new AutoWaveAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AutoWaveAudioProcessor();
}
