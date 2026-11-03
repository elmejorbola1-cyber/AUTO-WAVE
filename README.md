# AUTO WAVE — by KIDZEBA

Auto-tune en tiempo real (VST3) pensado para uso en vivo.

## Cómo actualizar tu repositorio de GitHub
Sube estos 9 archivos a la raíz de tu repositorio (Add file → Upload files). Los que ya existen se reemplazan.
El archivo `.github/workflows/build.yml` que ya tienes NO se toca.

    CMakeLists.txt        PluginProcessor.cpp   PluginProcessor.h
    PluginEditor.cpp      PluginEditor.h        AutoWaveLookAndFeel.h
    AutoTuneEngine.h      PitchDetector.h       PitchShifter.h

Al confirmar (Commit changes) GitHub compila solo. El resultado se llamará **AUTO WAVE.vst3**.

## Controles
- **Preset / Key / Scale / Voice Type / Mode** (barra superior)
- **TUNING**: Retune Speed, Flex-Tune, Humanize, Transpose
- **VIBRATO**: Vibrato (cantidad) y Vib Rate
- **OUTPUT**: Mix (seco/afinado) y Output
- **VOICE FX**: Low Layer, Doubler, Drive
- **SPACE**: Delay Time / FB / Mix, Reverb Size / Mix

Arriba a la derecha muestra la nota detectada y cuántos cents corrige.

## Modo Live vs Studio
- **Live** (por defecto): latencia mínima de proceso (~1 periodo de la voz).
- **Studio**: ventana el doble de larga, un poco más suave, el doble de latencia.

## Latencia (medida en pruebas offline del motor, 44.1 kHz)
| Modo | Latencia de proceso | Tiempo en "agarrar" una nota nueva |
|---|---|---|
| Live | ~1.7 ms (voz aguda) a ~9 ms (voz grave); ≈4.5 ms a 220 Hz | ≈30 ms |
| Studio | ~3.4 ms a ~18 ms; ≈9 ms a 220 Hz | ≈75 ms |

A eso se suma la latencia de tu interfaz/driver de audio: en FL Studio usa un driver ASIO y un buffer de 64–128 muestras.

## Presets
Zeba (Signature), Travis Scott, Jhay Weeler, Bad Bunny, Suei, Feid, Skaiy, Lil Juanse, Natural Correction, Hard Tune Clean.
Son aproximaciones propias inspiradas en el estilo de cada uno, no sus ajustes reales. Los presets no cambian Key, Scale, Voice Type ni Mode.

## Qué NO incluye (a diferencia del Auto-Tune EFX original)
Modelado de garganta/formantes (Throat Length, Formant) y Auto-Tune real de Antares: esto es una implementación propia.
