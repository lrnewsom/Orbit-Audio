#pragma once

#include <JuceHeader.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "Spatializer.h"

//==============================================================================
// I host the main UI and audio: device selector, spatializer controls, presets,
// reverb, and a "Run tests" button. The Spatializer does the DSP; I only pass
// parameters and run the reverb when enabled.
class MainComponent  : public juce::AudioAppComponent,
                       public juce::ChangeListener
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    juce::Point<int> getPreferredSize() const;
    void setOnPreferredSizeChanged (std::function<void()> callback);

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    //==============================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    //==============================================================================
    // I use atomics for values the audio thread reads so the UI can update them lock-free.

    juce::AudioDeviceSelectorComponent audioDeviceSelector;
    juce::Label panLabel;
    juce::Slider panSlider;
    std::atomic<float> panValue { 0.0f };

    juce::Slider panSpeedSlider;
    std::atomic<float> panSpeedHz { 0.05f };

    juce::Slider itdAmountSlider;
    juce::Label itdAmountLabel;
    std::atomic<float> itdAmount { 1.0f };
    juce::Slider shadowStrengthSlider;
    juce::Label shadowStrengthLabel;
    std::atomic<float> shadowStrength { 1.0f };

    juce::Slider depthSlider;
    juce::Label depthLabel;
    std::atomic<float> depth { 0.0f };
    juce::Slider widthSlider;
    juce::Label widthLabel;
    std::atomic<float> width { 1.0f };

    juce::ComboBox orbitModeCombo;
    std::atomic<int> orbitMode { 0 };

    juce::ToggleButton audioSettingsToggle { "Audio settings" };
    bool audioSettingsExpanded { false };

    juce::ComboBox presetCombo;
    juce::TextButton savePresetButton { "Save preset" };
    juce::TextButton quitButton { "Quit" };
    juce::TextButton runTestsButton { "Run tests" };
    juce::ToggleButton reverbToggle { "" };
    std::atomic<bool> reverbEnabled { false };
    juce::Slider reverbWetSlider;
    juce::Label reverbWetLabel;
    std::atomic<float> reverbWet { 0.33f };

    Spatializer spatializer;
    juce::Reverb reverb;

    juce::File getPresetsDirectory();
    juce::ValueTree getCurrentStateAsValueTree();
    void applyValueTreeToState (const juce::ValueTree& vt);
    void loadPreset (const juce::String& presetName);
    void savePreset (const juce::String& presetName);

    void changeListenerCallback (juce::ChangeBroadcaster* source) override;

    juce::File getAudioStateFile();
    void tryPreferLowLatencyBuffer();

    std::function<void()> onPreferredSizeChanged;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
