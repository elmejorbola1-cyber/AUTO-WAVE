#include "PluginEditor.h"
#include <cmath>

namespace
{
    juce::String noteName (float midi)
    {
        static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        const int m = (int) std::lround (midi);
        return juce::String (names[((m % 12) + 12) % 12]) + juce::String (m / 12 - 1);
    }
}

AutoWaveAudioProcessorEditor::AutoWaveAudioProcessorEditor (AutoWaveAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&laf);

    // --- selectores ---
    for (int i = 0; i < proc.getNumPrograms(); ++i)
        presetBox.addItem (proc.getProgramName (i), i + 1);
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.onChange = [this] { proc.setCurrentProgram (presetBox.getSelectedId() - 1); };
    addAndMakeVisible (presetBox);

    addChoice (keyBox,   "key",       keyAtt);
    addChoice (scaleBox, "scale",     scaleAtt);
    addChoice (inputBox, "inputType", inputAtt);
    addChoice (modeBox,  "mode",      modeAtt);

    captions.push_back ({ "PRESET",     &presetBox });
    captions.push_back ({ "KEY",        &keyBox });
    captions.push_back ({ "SCALE",      &scaleBox });
    captions.push_back ({ "VOICE TYPE", &inputBox });
    captions.push_back ({ "MODE",       &modeBox });

    // --- knobs (el orden define las secciones de abajo) ---
    addKnob ("retune",     "RETUNE SPEED");   // 0
    addKnob ("flex",       "FLEX-TUNE");      // 1
    addKnob ("humanize",   "HUMANIZE");       // 2
    addKnob ("transpose",  "TRANSPOSE");      // 3
    addKnob ("vibAmt",     "VIBRATO");        // 4
    addKnob ("vibRate",    "VIB RATE");       // 5
    addKnob ("mix",        "MIX");            // 6
    addKnob ("output",     "OUTPUT");         // 7
    addKnob ("lowLayer",   "LOW LAYER");      // 8
    addKnob ("doubler",    "DOUBLER");        // 9
    addKnob ("drive",      "DRIVE");          // 10
    addKnob ("delayTime",  "DELAY TIME");     // 11
    addKnob ("delayFb",    "DELAY FB");       // 12
    addKnob ("delayMix",   "DELAY MIX");      // 13
    addKnob ("reverbSize", "REVERB SIZE");    // 14
    addKnob ("reverbMix",  "REVERB MIX");     // 15

    sections.push_back ({ "TUNING",  0, 0, 4, 0,  {} });
    sections.push_back ({ "VIBRATO", 0, 4, 2, 4,  {} });
    sections.push_back ({ "OUTPUT",  0, 6, 2, 6,  {} });
    sections.push_back ({ "VOICE FX", 1, 0, 3, 8, {} });
    sections.push_back ({ "SPACE",   1, 3, 5, 11, {} });

    setSize (900, 570);
    startTimerHz (30);
}

AutoWaveAudioProcessorEditor::~AutoWaveAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void AutoWaveAudioProcessorEditor::addChoice (juce::ComboBox& box, const juce::String& id,
                                              std::unique_ptr<ComboAtt>& att)
{
    if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (id)))
        box.addItemList (c->choices, 1);
    addAndMakeVisible (box);
    att = std::make_unique<ComboAtt> (proc.apvts, id, box);
}

void AutoWaveAudioProcessorEditor::addKnob (const juce::String& id, const juce::String& text)
{
    auto k = std::make_unique<Knob>();
    k->text = text;
    k->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 84, 16);
    k->att = std::make_unique<SliderAtt> (proc.apvts, id, k->slider);
    addAndMakeVisible (k->slider);
    knobs.push_back (std::move (k));
}

void AutoWaveAudioProcessorEditor::timerCallback()
{
    wavePhase += 0.09;
    repaint (0, 0, getWidth(), headerH);
}

void AutoWaveAudioProcessorEditor::drawPlate (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& text,
                                              juce::Colour fill, float fontSize)
{
    g.setColour (fill);
    g.fillRoundedRectangle (r.toFloat(), 5.0f);
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (fontSize, juce::Font::bold));
    g.drawText (text, r, juce::Justification::centred, false);
}

void AutoWaveAudioProcessorEditor::drawWave (juce::Graphics& g, juce::Rectangle<float> area)
{
    juce::Graphics::ScopedSaveState ss (g);
    g.reduceClipRegion (area.toNearestInt());

    const float level = juce::jlimit (0.0f, 1.0f, proc.levelMeter.load());
    const float twoPi = juce::MathConstants<float>::twoPi;

    for (int layer = 0; layer < 3; ++layer)
    {
        const float amp  = area.getHeight() * (0.10f + 0.20f * level) * (1.0f - 0.22f * (float) layer);
        const float yMid = area.getBottom() - 26.0f + (float) layer * 5.0f;

        juce::Path p;
        p.startNewSubPath (area.getX(), area.getBottom());
        for (float x = 0.0f; x <= area.getWidth() + 3.0f; x += 3.0f)
        {
            const float t = x / area.getWidth();
            const float y = yMid + amp * std::sin (t * twoPi * (1.4f + 0.45f * (float) layer)
                                                   + (float) wavePhase * (1.0f + 0.35f * (float) layer));
            p.lineTo (area.getX() + x, y);
        }
        p.lineTo (area.getRight(), area.getBottom());
        p.closeSubPath();

        g.setColour (juce::Colours::white.withAlpha (0.16f + 0.10f * (float) layer));
        g.fillPath (p);
    }
}

void AutoWaveAudioProcessorEditor::paint (juce::Graphics& g)
{
    const float w = (float) getWidth();
    const float h = (float) getHeight();

    // fondo blanco
    g.setGradientFill (juce::ColourGradient (juce::Colours::white, 0.0f, (float) headerH,
                                             AW::bgLow, 0.0f, h, false));
    g.fillAll();

    // cabecera
    const juce::Rectangle<float> header (0.0f, 0.0f, w, (float) headerH);
    g.setGradientFill (juce::ColourGradient (AW::navy, 0.0f, 0.0f, AW::blue, w, (float) headerH, false));
    g.fillRect (header);
    g.setGradientFill (juce::ColourGradient (AW::cyan.withAlpha (0.45f), w * 0.85f, 0.0f,
                                             AW::cyan.withAlpha (0.0f), w * 0.5f, (float) headerH, true));
    g.fillRect (header);

    drawWave (g, header);

    // logo: círculo con ola
    {
        const juce::Rectangle<float> logo (26.0f, 24.0f, 62.0f, 62.0f);
        g.setColour (juce::Colours::white);
        g.drawEllipse (logo, 3.0f);
        juce::Path wv;
        for (float x = 0.0f; x <= 40.0f; x += 1.5f)
        {
            const float y = logo.getCentreY() + 8.0f * std::sin (x / 40.0f * juce::MathConstants<float>::twoPi);
            if (x == 0.0f) wv.startNewSubPath (logo.getX() + 11.0f, y);
            else           wv.lineTo (logo.getX() + 11.0f + x, y);
        }
        g.strokePath (wv, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // título
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (46.0f, juce::Font::bold));
    g.drawText ("AUTO WAVE", 104, 18, 420, 56, juce::Justification::centredLeft, false);
    g.setFont (juce::FontOptions (13.5f, juce::Font::bold));
    g.setColour (juce::Colours::white.withAlpha (0.9f));
    g.drawText ("PITCH CORRECTION   |   by KIDZEBA", 108, 72, 420, 20, juce::Justification::centredLeft, false);

    // lectura de nota
    {
        const float midi = proc.detectedMidi.load();
        const float corr = proc.correctionSemis.load();
        const juce::Rectangle<int> box ((int) w - 236, 22, 210, 62);
        g.setColour (juce::Colours::white.withAlpha (0.16f));
        g.fillRoundedRectangle (box.toFloat(), 10.0f);
        g.setColour (juce::Colours::white.withAlpha (0.35f));
        g.drawRoundedRectangle (box.toFloat(), 10.0f, 1.2f);

        g.setColour (juce::Colours::white);
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.drawText ("NOTE", box.getX() + 14, box.getY() + 8, 60, 14, juce::Justification::centredLeft, false);
        g.drawText ("CORRECTION", box.getX() + 104, box.getY() + 8, 96, 14, juce::Justification::centredLeft, false);

        g.setFont (juce::FontOptions (28.0f, juce::Font::bold));
        g.drawText (midi > 0.0f ? noteName (midi) : juce::String ("--"),
                    box.getX() + 14, box.getY() + 24, 90, 32, juce::Justification::centredLeft, false);

        const juce::String cents = midi > 0.0f
            ? (corr >= 0.0f ? "+" : "") + juce::String ((int) std::lround (corr * 100.0f)) + " ct"
            : juce::String ("--");
        g.setFont (juce::FontOptions (20.0f, juce::Font::bold));
        g.drawText (cents, box.getX() + 104, box.getY() + 28, 100, 26, juce::Justification::centredLeft, false);
    }

    // línea de acento
    g.setColour (AW::cyan);
    g.fillRect (0, headerH - 3, getWidth(), 3);

    // captions de los selectores
    for (auto& cap : captions)
    {
        const auto b = cap.second->getBounds();
        drawPlate (g, { b.getX(), b.getY() - 19, b.getWidth(), 16 }, cap.first, AW::blue, 10.5f);
    }

    // paneles
    for (auto& s : sections)
    {
        g.setColour (AW::panel);
        g.fillRoundedRectangle (s.bounds.toFloat(), 10.0f);
        g.setColour (AW::line);
        g.drawRoundedRectangle (s.bounds.toFloat(), 10.0f, 1.2f);
        drawPlate (g, { s.bounds.getX() + 8, s.bounds.getY() + 8, s.bounds.getWidth() - 16, 20 },
                   s.title, AW::navy, 12.0f);
    }

    // etiquetas de knobs
    for (auto& k : knobs)
        drawPlate (g, k->plate, k->text, AW::blue, 10.5f);
}

void AutoWaveAudioProcessorEditor::resized()
{
    const int margin = 16;

    // barra de selectores
    const int barY = headerH + 26;
    int x = margin;
    auto place = [&] (juce::ComboBox& b, int wdt) { b.setBounds (x, barY, wdt, 26); x += wdt + 12; };
    place (presetBox, 210);
    place (keyBox,    84);
    place (scaleBox,  170);
    place (inputBox,  160);
    place (modeBox,   110);

    // paneles y knobs
    auto body = getLocalBounds().withTrimmedTop (headerH + 64).reduced (margin, 6);
    const int rowH  = body.getHeight() / 2;
    const int cellW = body.getWidth() / 8;

    for (auto& s : sections)
    {
        s.bounds = juce::Rectangle<int> (body.getX() + s.startCol * cellW, body.getY() + s.row * rowH,
                                         s.count * cellW, rowH).reduced (5);
        auto inner = s.bounds.withTrimmedTop (32).reduced (4, 2);
        const int kw = inner.getWidth() / s.count;

        for (int i = 0; i < s.count; ++i)
        {
            auto& k = *knobs[(size_t) (s.first + i)];
            auto cell = juce::Rectangle<int> (inner.getX() + i * kw, inner.getY(), kw, inner.getHeight()).reduced (3, 0);
            k.plate = cell.removeFromTop (18);
            k.slider.setBounds (cell);
        }
    }
}
