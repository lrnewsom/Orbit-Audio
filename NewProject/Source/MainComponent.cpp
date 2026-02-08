#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
: audioDeviceSelector (deviceManager,
                        2, 2,  // min/max input channels
                        2, 2,  // min/max output channels
                        false, // no MIDI input
                        false, // no MIDI output
                        false, // no stereo pairs
                        false) // show buffer size/sample rate for low-latency tuning
{
    setSize (getPreferredSize().x, getPreferredSize().y);
    deviceManager.addChangeListener (this);

    std::unique_ptr<juce::XmlElement> savedState;
    if (getAudioStateFile().existsAsFile())
    {
        if (auto xml = juce::parseXML (getAudioStateFile()))
            savedState = std::move (xml);
    }

    const bool hadSavedState = (savedState != nullptr);
    if (juce::RuntimePermissions::isRequired (juce::RuntimePermissions::recordAudio)
        && ! juce::RuntimePermissions::isGranted (juce::RuntimePermissions::recordAudio))
    {
        juce::RuntimePermissions::request (juce::RuntimePermissions::recordAudio,
            [this, hadSavedState] (bool granted)
            {
                std::unique_ptr<juce::XmlElement> xml;
                if (granted && getAudioStateFile().existsAsFile())
                    xml = juce::parseXML (getAudioStateFile());
                setAudioChannels (granted ? 2 : 0, 2, xml.get());
                if (! hadSavedState) tryPreferLowLatencyBuffer();
            });
    }
    else
    {
        setAudioChannels (2, 2, savedState.get());
        if (! hadSavedState) tryPreferLowLatencyBuffer();
    }

    addAndMakeVisible (audioDeviceSelector);
    audioDeviceSelector.setVisible (false);

    audioSettingsToggle.setToggleState (false, juce::dontSendNotification);
    audioSettingsToggle.onClick = [this]
    {
        audioSettingsExpanded = audioSettingsToggle.getToggleState();
        audioDeviceSelector.setVisible (audioSettingsExpanded);
        setSize (getPreferredSize().x, getPreferredSize().y);
        resized();
        if (onPreferredSizeChanged)
            onPreferredSizeChanged();
    };
    addAndMakeVisible (audioSettingsToggle);
    audioSettingsToggle.setTooltip ("Expand to choose audio input/output devices, buffer size, and sample rate.");

    quitButton.onClick = []
    {
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    };
    addAndMakeVisible (quitButton);
    quitButton.setTooltip ("Quit OrbitAudio.");

    orbitModeCombo.addItem ("Manual", 1);
    orbitModeCombo.addItem ("Orbit (3D)", 2);
    orbitModeCombo.addItem ("Figure-8 (8D)", 3);
    orbitModeCombo.setSelectedId (1, juce::dontSendNotification);
    orbitModeCombo.onChange = [this]
    {
        int id = orbitModeCombo.getSelectedId();
        orbitMode.store (id - 1);
        panSlider.setEnabled (id == 1);
    };
    addAndMakeVisible (orbitModeCombo);
    orbitModeCombo.setTooltip ("Manual: use pan knob. Orbit: circular motion. Figure-8: tighter 8D-style orbit.");

    panSpeedSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    panSpeedSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 50, 20);
    panSpeedSlider.setRange (0.02, 0.5, 0.01);
    panSpeedSlider.setValue (0.05);
    panSpeedSlider.onValueChange = [this] { panSpeedHz.store ((float) panSpeedSlider.getValue()); };
    panSpeedSlider.setTooltip ("LFO speed for Orbit/Figure-8 modes (0.02 to 0.5 Hz).");
    addAndMakeVisible (panSpeedSlider);

    panLabel.setText ("Pan", juce::dontSendNotification);
    panLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (panLabel);
    panSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    panSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    panSlider.setRange (-1.0, 1.0, 0.001);
    panSlider.setValue (0.0);
    panSlider.onValueChange = [this] { panValue.store ((float) panSlider.getValue()); };
    panSlider.setTooltip ("Manual pan position: -1 = full left, +1 = full right. Disabled when Orbit or Figure-8 is active.");
    addAndMakeVisible (panSlider);

    itdAmountLabel.setText ("ITD", juce::dontSendNotification);
    itdAmountLabel.attachToComponent (&itdAmountSlider, true);
    itdAmountSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    itdAmountSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 50, 20);
    itdAmountSlider.setRange (0.0, 1.0, 0.01);
    itdAmountSlider.setValue (1.0);
    itdAmountSlider.onValueChange = [this]
    {
        float v = (float) itdAmountSlider.getValue();
        itdAmount.store (v);
        spatializer.setItdAmount (v);
    };
    itdAmountSlider.setTooltip ("Interaural time difference: delay on the far ear for directional feel (0 = none, 1 = full).");
    addAndMakeVisible (itdAmountSlider);

    shadowStrengthLabel.setText ("Shadow", juce::dontSendNotification);
    shadowStrengthLabel.attachToComponent (&shadowStrengthSlider, true);
    shadowStrengthSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    shadowStrengthSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 50, 20);
    shadowStrengthSlider.setRange (0.0, 1.0, 0.01);
    shadowStrengthSlider.setValue (1.0);
    shadowStrengthSlider.onValueChange = [this]
    {
        float v = (float) shadowStrengthSlider.getValue();
        shadowStrength.store (v);
        spatializer.setShadowStrength (v);
    };
    shadowStrengthSlider.setTooltip ("Head-shadow effect: low-pass filter on the far ear (0 = none, 1 = maximum).");
    addAndMakeVisible (shadowStrengthSlider);
    addAndMakeVisible (itdAmountLabel);
    addAndMakeVisible (shadowStrengthLabel);

    depthLabel.setText ("Depth", juce::dontSendNotification);
    depthLabel.attachToComponent (&depthSlider, true);
    depthSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    depthSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 50, 20);
    depthSlider.setRange (0.0, 1.0, 0.01);
    depthSlider.setValue (0.0);
    depthSlider.setTooltip ("HF rolloff for distance effect (0 = close, 1 = far)");
    depthSlider.onValueChange = [this]
    {
        float v = (float) depthSlider.getValue();
        depth.store (v);
        spatializer.setDepth (v);
    };
    addAndMakeVisible (depthSlider);
    addAndMakeVisible (depthLabel);

    widthLabel.setText ("Width", juce::dontSendNotification);
    widthLabel.attachToComponent (&widthSlider, true);
    widthSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    widthSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 50, 20);
    widthSlider.setRange (0.0, 1.0, 0.01);
    widthSlider.setValue (1.0);
    widthSlider.setTooltip ("Stereo field scale (0 = narrow, 1 = full)");
    widthSlider.onValueChange = [this]
    {
        float v = (float) widthSlider.getValue();
        width.store (v);
        spatializer.setWidth (v);
    };
    addAndMakeVisible (widthSlider);
    addAndMakeVisible (widthLabel);

    // Presets: I load from disk (OrbitAudio/Presets/*.xml) or use built-in values.
    presetCombo.addItem ("Default", 1);
    presetCombo.addItem ("Orbit", 2);
    presetCombo.addItem ("Wide", 3);
    presetCombo.addItem ("Narrow", 4);
    presetCombo.setSelectedId (1, juce::dontSendNotification);
    presetCombo.onChange = [this]
    {
        int id = presetCombo.getSelectedId();
        if (id == 1) loadPreset ("Default");
        else if (id == 2) loadPreset ("Orbit");
        else if (id == 3) loadPreset ("Wide");
        else if (id == 4) loadPreset ("Narrow");
    };
    presetCombo.setTooltip ("Quick preset for spatialization settings.");
    addAndMakeVisible (presetCombo);

    savePresetButton.setTooltip ("Save current settings to the selected preset (except Default).");
    savePresetButton.onClick = [this]
    {
        int id = presetCombo.getSelectedId();
        if (id == 2) savePreset ("Orbit");
        else if (id == 3) savePreset ("Wide");
        else if (id == 4) savePreset ("Narrow");
    };
    addAndMakeVisible (savePresetButton);

    // I run the "Audio" category unit tests and show the result in an alert.
    runTestsButton.onClick = [this]
    {
        juce::UnitTestRunner runner;
        runner.setPassesAreLogged (true);
        runner.runTestsInCategory ("Audio", 0);
        juce::String msg;
        int n = runner.getNumResults();
        int totalPass = 0, totalFail = 0;
        for (int i = 0; i < n; ++i)
        {
            if (auto* r = runner.getResult (i))
            {
                totalPass += r->passes;
                totalFail += r->failures;
                msg << r->unitTestName << " / " << r->subcategoryName
                    << ": " << r->passes << " pass, " << r->failures << " fail\n";
                for (const auto& m : r->messages)
                    msg << "  " << m << "\n";
            }
        }
        msg << "\nTotal: " << totalPass << " passed, " << totalFail << " failed.";
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon,
                                                "Unit tests", msg);
    };
#if JUCE_DEBUG
    runTestsButton.setTooltip ("Run audio unit tests (Debug builds only).");
    addAndMakeVisible (runTestsButton);
#endif

    reverbToggle.setToggleState (false, juce::dontSendNotification);
    reverbToggle.onClick = [this] { reverbEnabled.store (reverbToggle.getToggleState()); };
    reverbToggle.setTooltip ("Enable stereo reverb for added depth.");
    addAndMakeVisible (reverbToggle);

    reverbWetLabel.setText ("Reverb Wet", juce::dontSendNotification);
    reverbWetLabel.attachToComponent (&reverbWetSlider, true);
    reverbWetSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    reverbWetSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 40, 20);
    reverbWetSlider.setRange (0.0, 1.0, 0.01);
    reverbWetSlider.setValue (0.33);
    reverbWetSlider.onValueChange = [this]
    {
        float v = (float) reverbWetSlider.getValue();
        reverbWet.store (v);
        auto params = reverb.getParameters();
        params.wetLevel = v;
        reverb.setParameters (params);
    };
    reverbWetSlider.setTooltip ("Reverb wet mix when Reverb is on (0 = dry, 1 = full wet).");
    addAndMakeVisible (reverbWetSlider);
    addAndMakeVisible (reverbWetLabel);
}

MainComponent::~MainComponent()
{
    deviceManager.removeChangeListener (this);
    shutdownAudio();
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    juce::FloatVectorOperations::disableDenormalisedNumberSupport();
    spatializer.prepareToPlay (samplesPerBlockExpected, sampleRate);
    reverb.setSampleRate (sampleRate);
    auto params = reverb.getParameters();
    params.wetLevel = reverbWet.load();
    reverb.setParameters (params);
    reverb.reset();
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    // I run the spatializer with the current UI state, then optionally reverb.
    const int mode = orbitMode.load();
    const auto orbMode = mode == 0 ? Spatializer::OrbitMode::Manual
                      : mode == 1 ? Spatializer::OrbitMode::Orbit
                      : Spatializer::OrbitMode::Figure8;
    spatializer.process (*bufferToFill.buffer,
                         bufferToFill.startSample,
                         bufferToFill.numSamples,
                         panValue.load(),
                         orbMode,
                         panSpeedHz.load());

    if (reverbEnabled.load())
    {
        auto* left  = bufferToFill.buffer->getWritePointer (0, bufferToFill.startSample);
        auto* right = bufferToFill.buffer->getWritePointer (1, bufferToFill.startSample);
        reverb.processStereo (left, right, bufferToFill.numSamples);
    }
}

void MainComponent::releaseResources()
{
    // Called when the audio device stops or restarts; I don't need to free anything here.
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced (8);

    auto header = area.removeFromTop (28);
    audioSettingsToggle.setBounds (header.removeFromLeft (110).reduced (0, 4));
    quitButton.setBounds (header.removeFromRight (60).reduced (0, 4));

    if (audioSettingsExpanded)
    {
        auto deviceArea = area.removeFromTop (340);
        audioDeviceSelector.setBounds (deviceArea);
    }

    auto controls = area;
    const int labelWidth = 52;
    const int rowH = 32;

    auto row1 = controls.removeFromTop (56);
    auto panArea = row1.removeFromLeft (90);
    panLabel.setBounds (panArea.removeFromTop (18));
    panSlider.setBounds (panArea.reduced (4));
    orbitModeCombo.setBounds (row1.removeFromLeft (110).reduced (4, 8));
    panSpeedSlider.setBounds (row1.reduced (4, 8));

    auto row2 = controls.removeFromTop (rowH);
    itdAmountSlider.setBounds (row2.reduced (labelWidth, 4));

    auto row3 = controls.removeFromTop (rowH);
    shadowStrengthSlider.setBounds (row3.reduced (labelWidth, 4));

    auto row4 = controls.removeFromTop (rowH);
    depthSlider.setBounds (row4.reduced (labelWidth, 4));

    auto row5 = controls.removeFromTop (rowH);
    widthSlider.setBounds (row5.reduced (labelWidth, 4));

    auto row6 = controls.removeFromTop (rowH);
    presetCombo.setBounds (row6.removeFromLeft (80).reduced (2, 4));
    savePresetButton.setBounds (row6.removeFromLeft (70).reduced (2, 4));
#if JUCE_DEBUG
    runTestsButton.setBounds (row6.removeFromLeft (65).reduced (2, 4));
#endif
    reverbToggle.setBounds (row6.removeFromLeft (70).reduced (2, 4));
    reverbWetSlider.setBounds (row6.reduced (labelWidth, 4));
}

//==============================================================================
juce::File MainComponent::getPresetsDirectory()
{
    // I store presets under ~/Library/Application Support/OrbitAudio/Presets/.
    auto dir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                   .getChildFile ("OrbitAudio")
                   .getChildFile ("Presets");
    if (! dir.exists())
        dir.createDirectory();
    return dir;
}

juce::ValueTree MainComponent::getCurrentStateAsValueTree()
{
    juce::ValueTree vt ("OrbitAudioPreset");
    vt.setProperty ("pan", (double) panValue.load(), nullptr);
    vt.setProperty ("orbitMode", orbitMode.load(), nullptr);
    vt.setProperty ("panSpeedHz", (double) panSpeedHz.load(), nullptr);
    vt.setProperty ("itdAmount", (double) itdAmount.load(), nullptr);
    vt.setProperty ("shadowStrength", (double) shadowStrength.load(), nullptr);
    vt.setProperty ("depth", (double) depth.load(), nullptr);
    vt.setProperty ("width", (double) width.load(), nullptr);
    vt.setProperty ("reverbWet", (double) reverbWet.load(), nullptr);
    return vt;
}

void MainComponent::applyValueTreeToState (const juce::ValueTree& vt)
{
    if (! vt.isValid()) return;
    double pan = vt.getProperty ("pan", 0.0);
    int orbMode = vt.getProperty ("orbitMode", 0);
    if (vt.getProperty ("autoPan", false)) orbMode = 1;
    double speed = vt.getProperty ("panSpeedHz", 0.05);
    double itd = vt.getProperty ("itdAmount", 1.0);
    double shadow = vt.getProperty ("shadowStrength", 1.0);
    double dep = vt.getProperty ("depth", 0.0);
    double wid = vt.getProperty ("width", 1.0);
    double rvbWet = vt.getProperty ("reverbWet", 0.33);

    panValue.store ((float) pan);
    panSlider.setValue (pan, juce::dontSendNotification);
    orbitMode.store (orbMode);
    orbitModeCombo.setSelectedId (orbMode + 1, juce::dontSendNotification);
    panSlider.setEnabled (orbMode == 0);
    panSpeedHz.store ((float) speed);
    panSpeedSlider.setValue (speed, juce::dontSendNotification);
    itdAmount.store ((float) itd);
    itdAmountSlider.setValue (itd, juce::dontSendNotification);
    spatializer.setItdAmount ((float) itd);
    shadowStrength.store ((float) shadow);
    shadowStrengthSlider.setValue (shadow, juce::dontSendNotification);
    spatializer.setShadowStrength ((float) shadow);
    depth.store ((float) dep);
    depthSlider.setValue (dep, juce::dontSendNotification);
    spatializer.setDepth ((float) dep);
    width.store ((float) wid);
    widthSlider.setValue (wid, juce::dontSendNotification);
    spatializer.setWidth ((float) wid);
    reverbWet.store ((float) rvbWet);
    reverbWetSlider.setValue (rvbWet, juce::dontSendNotification);
    auto params = reverb.getParameters();
    params.wetLevel = (float) rvbWet;
    reverb.setParameters (params);
}

void MainComponent::loadPreset (const juce::String& presetName)
{
    // I reset to neutral values for Default; otherwise I load from file or use built-ins.
    if (presetName == "Default")
    {
        panValue.store (0.0f);
        panSlider.setValue (0.0, juce::dontSendNotification);
        orbitMode.store (0);
        orbitModeCombo.setSelectedId (1, juce::dontSendNotification);
        panSlider.setEnabled (true);
        panSpeedHz.store (0.05f);
        panSpeedSlider.setValue (0.05, juce::dontSendNotification);
        itdAmount.store (1.0f);
        itdAmountSlider.setValue (1.0, juce::dontSendNotification);
        spatializer.setItdAmount (1.0f);
        shadowStrength.store (1.0f);
        shadowStrengthSlider.setValue (1.0, juce::dontSendNotification);
        spatializer.setShadowStrength (1.0f);
        depth.store (0.0f);
        depthSlider.setValue (0.0, juce::dontSendNotification);
        spatializer.setDepth (0.0f);
        width.store (1.0f);
        widthSlider.setValue (1.0, juce::dontSendNotification);
        spatializer.setWidth (1.0f);
        reverbWet.store (0.33f);
        reverbWetSlider.setValue (0.33, juce::dontSendNotification);
        auto params = reverb.getParameters();
        params.wetLevel = 0.33f;
        reverb.setParameters (params);
        return;
    }

    juce::File file = getPresetsDirectory().getChildFile (presetName + ".xml");
    if (file.existsAsFile())
    {
        if (auto xml = juce::parseXML (file))
        {
            juce::ValueTree vt = juce::ValueTree::fromXml (*xml);
            applyValueTreeToState (vt);
            return;
        }
    }

    juce::ValueTree vt ("OrbitAudioPreset");
    if (presetName == "Orbit")
    {
        vt.setProperty ("pan", 0.0, nullptr);
        vt.setProperty ("orbitMode", 1, nullptr);
        vt.setProperty ("panSpeedHz", 0.05, nullptr);
        vt.setProperty ("itdAmount", 1.0, nullptr);
        vt.setProperty ("shadowStrength", 1.0, nullptr);
        vt.setProperty ("depth", 0.0, nullptr);
        vt.setProperty ("width", 1.0, nullptr);
        vt.setProperty ("reverbWet", 0.33, nullptr);
    }
    else if (presetName == "Wide")
    {
        vt.setProperty ("pan", 0.0, nullptr);
        vt.setProperty ("orbitMode", 0, nullptr);
        vt.setProperty ("panSpeedHz", 0.05, nullptr);
        vt.setProperty ("itdAmount", 0.5, nullptr);
        vt.setProperty ("shadowStrength", 0.5, nullptr);
        vt.setProperty ("depth", 0.0, nullptr);
        vt.setProperty ("width", 1.0, nullptr);
        vt.setProperty ("reverbWet", 0.33, nullptr);
    }
    else if (presetName == "Narrow")
    {
        vt.setProperty ("pan", 0.0, nullptr);
        vt.setProperty ("orbitMode", 2, nullptr);
        vt.setProperty ("panSpeedHz", 0.1, nullptr);
        vt.setProperty ("itdAmount", 1.0, nullptr);
        vt.setProperty ("shadowStrength", 1.0, nullptr);
        vt.setProperty ("depth", 0.2, nullptr);
        vt.setProperty ("width", 0.7, nullptr);
        vt.setProperty ("reverbWet", 0.25, nullptr);
    }
    applyValueTreeToState (vt);
}

void MainComponent::savePreset (const juce::String& presetName)
{
    if (presetName == "Default") return;
    juce::ValueTree vt = getCurrentStateAsValueTree();
    vt.setProperty ("name", presetName, nullptr);
    juce::File file = getPresetsDirectory().getChildFile (presetName + ".xml");
    if (auto xml = vt.createXml())
        xml->writeTo (file);
}

juce::File MainComponent::getAudioStateFile()
{
    return getPresetsDirectory().getParentDirectory().getChildFile ("audioDeviceState.xml");
}

void MainComponent::changeListenerCallback (juce::ChangeBroadcaster*)
{
    if (auto xml = deviceManager.createStateXml())
        xml->writeTo (getAudioStateFile());
}

juce::Point<int> MainComponent::getPreferredSize() const
{
    return { 540, audioSettingsExpanded ? 660 : 380 };
}

void MainComponent::setOnPreferredSizeChanged (std::function<void()> callback)
{
    onPreferredSizeChanged = std::move (callback);
}

void MainComponent::tryPreferLowLatencyBuffer()
{
    auto* device = deviceManager.getCurrentAudioDevice();
    if (device == nullptr) return;

    auto setup = deviceManager.getAudioDeviceSetup();
    if (setup.bufferSize > 0 && setup.bufferSize <= 256) return;

    auto sizes = device->getAvailableBufferSizes();
    int best = 0;
    for (auto s : sizes)
        if (s <= 128 && s > best) best = s;
    if (best == 0)
        for (auto s : sizes)
            if (s <= 256 && (best == 0 || s < best)) best = s;

    if (best > 0)
    {
        setup.bufferSize = best;
        deviceManager.setAudioDeviceSetup (setup, true);
    }
}
