#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace AW
{
    const juce::Colour navy    = juce::Colour (0xff0b1f4d);
    const juce::Colour blue    = juce::Colour (0xff1d6fe0);
    const juce::Colour cyan    = juce::Colour (0xff3ec1f0);
    const juce::Colour panel   = juce::Colour (0xffeef2f8);
    const juce::Colour line    = juce::Colour (0xffd3dbe8);
    const juce::Colour bgLow   = juce::Colour (0xffe9eef7);
}

class AutoWaveLookAndFeel : public juce::LookAndFeel_V4
{
public:
    AutoWaveLookAndFeel()
    {
        setColour (juce::Slider::textBoxTextColourId,        AW::navy);
        setColour (juce::Slider::textBoxBackgroundColourId,  juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxOutlineColourId,     juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxHighlightColourId,   AW::blue.withAlpha (0.3f));

        setColour (juce::ComboBox::backgroundColourId,       AW::navy);
        setColour (juce::ComboBox::textColourId,             juce::Colours::white);
        setColour (juce::ComboBox::arrowColourId,            juce::Colours::white);
        setColour (juce::ComboBox::outlineColourId,          AW::navy);

        setColour (juce::PopupMenu::backgroundColourId,            AW::navy);
        setColour (juce::PopupMenu::textColourId,                  juce::Colours::white);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, AW::blue);
        setColour (juce::PopupMenu::highlightedTextColourId,       juce::Colours::white);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float startAngle, float endAngle, juce::Slider&) override
    {
        auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (5.0f);
        const float r = std::min (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto  c = bounds.getCentre();
        const float angle = startAngle + sliderPos * (endAngle - startAngle);

        // anillo de valor
        juce::Path track;
        track.addCentredArc (c.x, c.y, r, r, 0.0f, startAngle, endAngle, true);
        g.setColour (AW::line);
        g.strokePath (track, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        juce::Path val;
        val.addCentredArc (c.x, c.y, r, r, 0.0f, startAngle, angle, true);
        g.setGradientFill (juce::ColourGradient (AW::blue, c.x - r, c.y, AW::cyan, c.x + r, c.y, false));
        g.strokePath (val, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // cuerpo del knob (blanco metálico)
        const float kr = r * 0.72f;
        g.setColour (juce::Colours::black.withAlpha (0.14f));
        g.fillEllipse (c.x - kr, c.y - kr + 2.5f, kr * 2.0f, kr * 2.0f);

        g.setGradientFill (juce::ColourGradient (juce::Colours::white, c.x, c.y - kr,
                                                 juce::Colour (0xffc7d1e1), c.x, c.y + kr, false));
        g.fillEllipse (c.x - kr, c.y - kr, kr * 2.0f, kr * 2.0f);

        g.setColour (juce::Colour (0xffa9b6ca));
        g.drawEllipse (c.x - kr, c.y - kr, kr * 2.0f, kr * 2.0f, 1.2f);

        g.setColour (juce::Colours::white.withAlpha (0.9f));
        g.drawEllipse (c.x - kr + 3.0f, c.y - kr + 3.0f, kr * 2.0f - 6.0f, kr * 2.0f - 6.0f, 1.0f);

        // aguja
        juce::Path p;
        p.addRoundedRectangle (-2.0f, -kr + 4.0f, 4.0f, kr * 0.45f, 2.0f);
        g.setColour (AW::navy);
        g.fillPath (p, juce::AffineTransform::rotation (angle).translated (c.x, c.y));
    }
};
