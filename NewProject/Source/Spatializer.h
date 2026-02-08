#pragma once

#include <JuceHeader.h>

//==============================================================================
// I do binaural-style stereo spatialization: pan + ITD (interaural time difference)
// + a head-shadow low-pass on the far ear. I support 3D/8D-style effects: depth
// (HF rolloff for distance), width (stereo field scale), and multiple orbit modes.
// I'm designed to run on the audio thread only with minimal latency.
class Spatializer
{
public:
    enum class OrbitMode { Manual, Orbit, Figure8 };

    Spatializer();
    ~Spatializer() = default;

    // I reset my delay/LPF/LFO state when sample rate (or block size) changes.
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate);

    // I process a block of stereo audio in-place. If orbitMode != Manual I drive
    // pan from an internal LFO at panSpeedHz; otherwise I use manualPan (-1 = left, +1 = right).
    void process (juce::AudioBuffer<float>& buffer,
                  int startSample,
                  int numSamples,
                  float manualPan,
                  OrbitMode orbitMode,
                  float panSpeedHz);

    // I scale the interaural delay: 0 = none, 1 = full (default 1).
    void setItdAmount (float amount);
    float getItdAmount() const { return itdAmount; }

    // I scale the head-shadow LPF: 0 = none, 1 = max (default 1).
    void setShadowStrength (float strength);
    float getShadowStrength() const { return shadowStrength; }

    // Depth 0–1: applies HF rolloff to simulate distance (air absorption).
    void setDepth (float depth);
    float getDepth() const { return depth; }

    // Width 0–1: scales stereo field / ILD intensity (0 = narrow, 1 = full).
    void setWidth (float width);
    float getWidth() const { return width; }

private:
    static constexpr int maxDelaySamples = 64;
    static constexpr float maxDelayMs = 0.5f;
    static constexpr float minCutoffHz = 2000.0f;
    static constexpr float maxCutoffHz = 18000.0f;

    double sampleRate = 44100.0;
    double lfoPhase = 0.0;

    // I use short delay lines for ITD: delay the “far” ear so the sound feels off to one side.
    float leftDelayBuffer[maxDelaySamples] = {};
    float rightDelayBuffer[maxDelaySamples] = {};
    int delayWriteIndex = 0;

    // I use one-pole LPFs on the far ear to mimic head shadow.
    float leftLPF = 0.0f;
    float rightLPF = 0.0f;

    std::atomic<float> itdAmount { 1.0f };
    std::atomic<float> shadowStrength { 1.0f };
    std::atomic<float> depth { 0.0f };
    std::atomic<float> width { 1.0f };

    // One-pole LPFs for depth (distance) HF rolloff.
    float depthLPF_L = 0.0f;
    float depthLPF_R = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Spatializer)
};
