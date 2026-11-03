#pragma once
#include <vector>
#include <cmath>
#include <algorithm>

// Pitch shifter de dominio temporal: dos lecturas con ventana Hann sobre una línea de retardo.
// La ventana se ajusta a un múltiplo de 2 periodos de la voz -> las dos lecturas quedan en fase.
// Latencia de proceso = window / 2  (≈ 1 periodo de la voz en modo Live).
class PitchShifter
{
public:
    void prepare (double sampleRate)
    {
        bufSize = 1 << 14;
        mask = bufSize - 1;
        buf.assign ((size_t) bufSize, 0.0f);
        writePos = 0;
        phase = 0.0f;
        window = targetWindow = 400.0f * (float) (sampleRate / 44100.0);
    }

    void  setWindow (float w) { targetWindow = std::min (6000.0f, std::max (96.0f, w)); }
    float getLatencySamples() const { return 0.5f * window; }

    float process (float in, float ratio)
    {
        buf[(size_t) writePos] = in;

        window += 0.002f * (targetWindow - window);

        phase += (1.0f - ratio) / window;
        phase -= std::floor (phase);

        float p2 = phase + 0.5f;
        if (p2 >= 1.0f) p2 -= 1.0f;

        const float w1 = 0.5f - 0.5f * std::cos (6.2831853f * phase);
        const float w2 = 0.5f - 0.5f * std::cos (6.2831853f * p2);

        const float out = w1 * read (phase * window) + w2 * read (p2 * window);

        writePos = (writePos + 1) & mask;
        return out;
    }

private:
    // Interpolación cúbica (Catmull-Rom): conserva mejor los agudos que la lineal
    float read (float delay) const
    {
        float pos = (float) writePos - delay;
        if (pos < 0.0f) pos += (float) bufSize;
        const int   i1 = (int) pos;
        const float t  = pos - (float) i1;
        const float y0 = buf[(size_t) ((i1 - 1) & mask)];
        const float y1 = buf[(size_t) (i1 & mask)];
        const float y2 = buf[(size_t) ((i1 + 1) & mask)];
        const float y3 = buf[(size_t) ((i1 + 2) & mask)];
        return y1 + 0.5f * t * (y2 - y0 + t * (2.0f * y0 - 5.0f * y1 + 4.0f * y2 - y3 + t * (3.0f * (y1 - y2) + y3 - y0)));
    }

    std::vector<float> buf;
    int   bufSize = 1 << 14, mask = (1 << 14) - 1, writePos = 0;
    float phase = 0.0f, window = 400.0f, targetWindow = 400.0f;
};
