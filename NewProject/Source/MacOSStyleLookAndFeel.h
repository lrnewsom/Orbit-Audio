/*
  ==============================================================================
    macOS-only LookAndFeel: liquid glass style and system font.
    Only include and use when JUCE_MAC is defined.
  ==============================================================================
*/

#pragma once

#if JUCE_MAC

#include <JuceHeader.h>

//==============================================================================
/** LookAndFeel that approximates macOS "liquid glass" UI: light gray background,
    white surfaces, blue accent, and system sans-serif font.
*/
class MacOSStyleLookAndFeel : public juce::LookAndFeel_V4
{
public:
    MacOSStyleLookAndFeel()
        : LookAndFeel_V4 (getLiquidGlassColourScheme())
    {}

    juce::Font getLabelFont (juce::Label& label) override
    {
        const float h = label.getFont().getHeight();
        return juce::Font (juce::FontOptions (juce::Font::getDefaultSansSerifFontName(),
                                               juce::jmax (11.0f, h),
                                               juce::Font::plain));
    }

    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override
    {
        const float size = juce::jmin (15.0f, (float) buttonHeight * 0.6f);
        return juce::Font (juce::FontOptions (juce::Font::getDefaultSansSerifFontName(),
                                               size,
                                               juce::Font::plain));
    }

    juce::Font getComboBoxFont (juce::ComboBox& box) override
    {
        const float size = juce::jmin (15.0f, (float) box.getHeight() * 0.85f);
        return juce::Font (juce::FontOptions (juce::Font::getDefaultSansSerifFontName(),
                                               size,
                                               juce::Font::plain));
    }

private:
    static juce::LookAndFeel_V4::ColourScheme getLiquidGlassColourScheme()
    {
        // Light glass-style palette: gray background, white surfaces, blue accent
        return {
            0xfff5f5f7,  // windowBackground  – light gray (Apple-style)
            0xffffffff,  // widgetBackground  – white
            0xffffffff,  // menuBackground
            0xffe5e5ea,  // outline           – light gray border
            0xff000000,  // defaultText       – black
            0xffd1d1d6,  // defaultFill       – gray fill
            0xffffffff,  // highlightedText   – white on blue
            0xff007aff,  // highlightedFill   – Apple blue accent
            0xff000000   // menuText
        };
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MacOSStyleLookAndFeel)
};

#endif // JUCE_MAC
