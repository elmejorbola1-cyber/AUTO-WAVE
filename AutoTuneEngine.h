#pragma once
#include <vector>
#include <cmath>
#include <algorithm>
#include "PitchDetector.h"
#include "PitchShifter.h"

struct TuneSettings
{
    float    retuneMs   = 20.0f;   // 0 = corrección instantánea
    float    flexCents  = 0.0f;
    float    humanize   = 0.0f;    // 0..100
    float    transpose  = 0.0f;    // semitonos
    float    vibDepth   = 0.0f;    // semitonos (pico)
    float    vibRate    = 5.5f;    // Hz
    float    mix        = 1.0f;    // 0..1
    float    lowAmt     = 0.0f;    // 0..1
    int      key        = 0;
    unsigned scaleMask  = 0xFFF;
    int      windowPeriods = 1;    // 1 = Live (mínima latencia), 2 = Studio (más suave)
};

class AutoTuneEngine
{
public:
    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        detector.prepare (sampleRate);
        shifter.prepare (sampleRate);
        lowShifter.prepare (sampleRate);
        dryBuf.assign (1 << 15, 0.0f);
        dryMask = (1 << 15) - 1;
        dryPos = 0;
        corr = targetCorr = vibPhase = vibEnv = 0.0f;
        voiced = false; unvoicedHops = 0; lastMidi = 0.0f;
        update (TuneSettings());
    }

    void setInputRange (float minHz, float maxHz) { detector.setRange (minHz, maxHz); }

    // Llamar una vez por bloque
    void update (const TuneSettings& s)
    {
        set = s;
        const float effMs = s.retuneMs + s.humanize * 0.8f;
        alphaTune = effMs <= 0.01f ? 1.0f : 1.0f - std::exp (-1.0f / (effMs * 0.001f * sr));
        alphaRel  = 1.0f - std::exp (-1.0f / (0.02f * sr));
        alphaVib  = 1.0f - std::exp (-1.0f / (0.2f * sr));
    }

    float process (float in)
    {
        if (detector.pushSample (in))
        {
            if (detector.isVoiced())
            {
                unvoicedHops = 0;
                voiced = true;
                const float f0 = detector.getF0();
                lastMidi = 69.0f + 12.0f * std::log2 (f0 / 440.0f);
                const int target = nearestAllowed (lastMidi, set.key, set.scaleMask);
                float c = (float) target - lastMidi;
                if (std::abs (c) * 100.0f <= set.flexCents) c = 0.0f;   // Flex-Tune
                targetCorr = c;

                const float w = 2.0f * (float) set.windowPeriods * (sr / f0);
                shifter.setWindow (w);
                lowShifter.setWindow (w);
            }
            else if (++unvoicedHops > 16)
                voiced = false;
        }

        corr += (voiced ? alphaTune : alphaRel) * ((voiced ? targetCorr : 0.0f) - corr);

        vibPhase += set.vibRate / sr;
        vibPhase -= std::floor (vibPhase);
        vibEnv += alphaVib * ((voiced ? 1.0f : 0.0f) - vibEnv);
        const float vib = set.vibDepth * vibEnv * std::sin (6.2831853f * vibPhase);

        const float ratio = std::exp2 ((corr + set.transpose + vib) / 12.0f);
        const float wet = shifter.process (in, ratio);
        const float low = lowShifter.process (wet, 0.5f);

        // Señal seca alineada con la latencia del shifter (para el control Mix)
        dryBuf[(size_t) dryPos] = in;
        float rp = (float) dryPos - shifter.getLatencySamples();
        if (rp < 0.0f) rp += (float) (dryMask + 1);
        const int i0 = (int) rp;
        const float fr = rp - (float) i0;
        const float a = dryBuf[(size_t) (i0 & dryMask)], b = dryBuf[(size_t) ((i0 + 1) & dryMask)];
        const float dry = a + fr * (b - a);
        dryPos = (dryPos + 1) & dryMask;

        return dry + set.mix * (wet - dry) + set.lowAmt * 0.8f * low;
    }

    bool  isVoiced() const { return voiced; }
    float getDetectedMidi() const { return voiced ? lastMidi : 0.0f; }
    float getCorrectionSemis() const { return corr; }
    float getLatencySamples() const { return shifter.getLatencySamples(); }

private:
    static int nearestAllowed (float midiF, int key, unsigned mask)
    {
        const int base = (int) std::floor (midiF);
        float best = 1.0e9f;
        int bestN = base;
        for (int n = base - 6; n <= base + 7; ++n)
        {
            const int pc = ((n - key) % 12 + 12) % 12;
            if (mask & (1u << pc))
            {
                const float d = std::abs ((float) n - midiF);
                if (d < best) { best = d; bestN = n; }
            }
        }
        return bestN;
    }

    PitchDetector detector;
    PitchShifter  shifter, lowShifter;
    TuneSettings  set;
    std::vector<float> dryBuf;
    int   dryMask = (1 << 15) - 1, dryPos = 0, unvoicedHops = 0;
    float sr = 44100.0f, alphaTune = 1.0f, alphaRel = 0.01f, alphaVib = 0.0001f;
    float corr = 0.0f, targetCorr = 0.0f, vibPhase = 0.0f, vibEnv = 0.0f, lastMidi = 0.0f;
    bool  voiced = false;
};
