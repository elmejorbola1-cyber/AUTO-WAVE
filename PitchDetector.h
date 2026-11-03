#pragma once
#include <vector>
#include <cmath>
#include <algorithm>

// Detector de tono monofónico tipo YIN sobre señal diezmada (~11 kHz).
// Ventana corta y ajustable por rango de voz = seguimiento rápido (clave para uso en vivo).
class PitchDetector
{
public:
    void prepare (double sampleRate)
    {
        decim       = std::max (1, (int) std::round (sampleRate / 11025.0));
        dsr         = (float) (sampleRate / decim);
        allocMaxTau = (int) (dsr / 60.0f) + 1;
        bufLen      = 2 * allocMaxTau + 4;

        ring.assign ((size_t) bufLen, 0.0f);
        frame.assign ((size_t) bufLen, 0.0f);
        diff.assign ((size_t) allocMaxTau + 3, 0.0f);
        cmnd.assign ((size_t) allocMaxTau + 3, 0.0f);

        writePos = 0; acc = 0.0f; accCount = 0; hopCount = 0;
        f0 = 0.0f; voiced = false;
        setRange (80.0f, 700.0f);
    }

    // Seguro de llamar desde el hilo de audio (no reserva memoria)
    void setRange (float minHz, float maxHz)
    {
        minTau = std::max (2, (int) (dsr / maxHz));
        maxTau = std::min (allocMaxTau, (int) (dsr / minHz));
        maxTau = std::max (maxTau, minTau + 3);
        win    = maxTau;
    }

    // true cuando hay un resultado nuevo (cada 16 muestras diezmadas ≈ 1.5 ms)
    bool pushSample (float x)
    {
        acc += x;
        if (++accCount < decim) return false;

        ring[(size_t) writePos] = acc / (float) decim;
        acc = 0.0f; accCount = 0;
        writePos = (writePos + 1) % bufLen;

        if (++hopCount < hop) return false;
        hopCount = 0;
        analyse();
        return true;
    }

    bool  isVoiced() const { return voiced; }
    float getF0()    const { return f0; }

private:
    void analyse()
    {
        const int len = win + maxTau + 2;
        const int start = writePos - len;
        float energy = 0.0f;
        for (int i = 0; i < len; ++i)
        {
            const float v = ring[(size_t) (((start + i) % bufLen + bufLen) % bufLen)];
            frame[(size_t) i] = v;
            energy += v * v;
        }
        if (std::sqrt (energy / (float) len) < 0.002f) { voiced = false; return; }

        for (int tau = 1; tau <= maxTau; ++tau)
        {
            float s = 0.0f;
            for (int j = 0; j < win; ++j)
            {
                const float d = frame[(size_t) j] - frame[(size_t) (j + tau)];
                s += d * d;
            }
            diff[(size_t) tau] = s;
        }

        cmnd[0] = 1.0f;
        float running = 0.0f;
        for (int tau = 1; tau <= maxTau; ++tau)
        {
            running += diff[(size_t) tau];
            cmnd[(size_t) tau] = running > 1.0e-12f ? diff[(size_t) tau] * (float) tau / running : 1.0f;
        }

        int tauEst = -1;
        for (int tau = minTau; tau < maxTau; ++tau)
        {
            if (cmnd[(size_t) tau] < threshold)
            {
                while (tau + 1 < maxTau && cmnd[(size_t) tau + 1] < cmnd[(size_t) tau]) ++tau;
                tauEst = tau;
                break;
            }
        }

        if (tauEst < 0) { voiced = false; return; }

        float t = (float) tauEst;
        const float a = cmnd[(size_t) tauEst - 1], b = cmnd[(size_t) tauEst], c = cmnd[(size_t) tauEst + 1];
        const float denom = a - 2.0f * b + c;
        if (std::abs (denom) > 1.0e-9f) t += 0.5f * (a - c) / denom;

        f0 = dsr / t;
        voiced = true;
    }

    std::vector<float> ring, frame, diff, cmnd;
    int   decim = 4, minTau = 11, maxTau = 137, win = 137, allocMaxTau = 184, bufLen = 372;
    int   writePos = 0, accCount = 0, hopCount = 0;
    const int hop = 16;
    float dsr = 11025.0f, acc = 0.0f, f0 = 0.0f;
    const float threshold = 0.15f;
    bool  voiced = false;
};
