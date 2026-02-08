#include "Spatializer.h"

//==============================================================================
Spatializer::Spatializer() = default;

//==============================================================================
void Spatializer::prepareToPlay (int /*samplesPerBlockExpected*/, double sampleRateIn)
{
    sampleRate = sampleRateIn;
    lfoPhase = 0.0;
    std::fill (std::begin (leftDelayBuffer),  std::end (leftDelayBuffer),  0.0f);
    std::fill (std::begin (rightDelayBuffer), std::end (rightDelayBuffer), 0.0f);
    leftLPF = rightLPF = 0.0f;
    depthLPF_L = depthLPF_R = 0.0f;
    delayWriteIndex = 0;
}

void Spatializer::setDepth (float d)
{
    depth.store (juce::jlimit (0.0f, 1.0f, d));
}

void Spatializer::setWidth (float w)
{
    width.store (juce::jlimit (0.0f, 1.0f, w));
}

//==============================================================================
void Spatializer::setItdAmount (float amount)
{
    itdAmount.store (juce::jlimit (0.0f, 1.0f, amount));
}

void Spatializer::setShadowStrength (float strength)
{
    shadowStrength.store (juce::jlimit (0.0f, 1.0f, strength));
}

//==============================================================================
void Spatializer::process (juce::AudioBuffer<float>& buffer,
                           int startSample,
                           int numSamples,
                           float manualPan,
                           OrbitMode orbitMode,
                           float panSpeedHz)
{
    const float itd = itdAmount.load();
    const float shadow = shadowStrength.load();
    const float depthVal = depth.load();
    const float widthVal = width.load();

    int maxDelaySamplesUsed = (int) (sampleRate * maxDelayMs / 1000.0f * itd);
    maxDelaySamplesUsed = juce::jmin (maxDelaySamplesUsed, maxDelaySamples);

    float pan = 0.0f;

    if (orbitMode == OrbitMode::Orbit)
    {
        pan = (float) std::sin (lfoPhase);
    }
    else if (orbitMode == OrbitMode::Figure8)
    {
        pan = (float) std::sin (2.0 * lfoPhase);
    }
    else
    {
        pan = manualPan;
    }

    if (orbitMode != OrbitMode::Manual)
    {
        const double phaseIncrement =
            juce::MathConstants<double>::twoPi * (double) panSpeedHz / sampleRate;
        lfoPhase += phaseIncrement * numSamples;
        if (lfoPhase > juce::MathConstants<double>::twoPi)
            lfoPhase -= juce::MathConstants<double>::twoPi;
    }

    const int leftDelaySamples  = pan > 0.0f ? (int) (pan * maxDelaySamplesUsed) : 0;
    const int rightDelaySamples = pan < 0.0f ? (int) (-pan * maxDelaySamplesUsed) : 0;

    const float shadowAmount = std::abs (pan) * shadow;
    const float cutoffHz = juce::jmap (1.0f - shadowAmount, 0.0f, 1.0f, minCutoffHz, maxCutoffHz);
    const float alpha = std::exp (-2.0f * juce::MathConstants<float>::pi * cutoffHz / (float) sampleRate);

    float leftGain  = std::cos ((pan + 1.0f) * juce::MathConstants<float>::halfPi * 0.5f);
    float rightGain = std::sin ((pan + 1.0f) * juce::MathConstants<float>::halfPi * 0.5f);

    if (widthVal < 1.0f)
    {
        const float mid = 0.5f * (leftGain + rightGain);
        const float side = 0.5f * (rightGain - leftGain);
        leftGain  = mid - side * widthVal;
        rightGain = mid + side * widthVal;
    }

    const float depthCutoffHz = depthVal > 0.0f
        ? juce::jmap (depthVal, 0.0f, 1.0f, 18000.0f, 1500.0f)
        : 18000.0f;
    const float depthAlpha = depthVal > 0.0f
        ? std::exp (-2.0f * juce::MathConstants<float>::pi * depthCutoffHz / (float) sampleRate)
        : 0.0f;

    auto* left  = buffer.getWritePointer (0, startSample);
    auto* right = buffer.getWritePointer (1, startSample);

    for (int i = 0; i < numSamples; ++i)
    {
        float inL = left[i];
        float inR = right[i];

        if (depthVal > 0.0f)
        {
            depthLPF_L = depthAlpha * depthLPF_L + (1.0f - depthAlpha) * inL;
            depthLPF_R = depthAlpha * depthLPF_R + (1.0f - depthAlpha) * inR;
            inL = juce::jmap (depthVal, 0.0f, 1.0f, inL, depthLPF_L);
            inR = juce::jmap (depthVal, 0.0f, 1.0f, inR, depthLPF_R);
        }

        leftDelayBuffer[delayWriteIndex]  = inL;
        rightDelayBuffer[delayWriteIndex] = inR;

        const int leftReadIndex  = (delayWriteIndex - leftDelaySamples + maxDelaySamples) % maxDelaySamples;
        const int rightReadIndex = (delayWriteIndex - rightDelaySamples + maxDelaySamples) % maxDelaySamples;

        float outL = leftDelayBuffer[leftReadIndex]  * leftGain;
        float outR = rightDelayBuffer[rightReadIndex] * rightGain;

        if (pan > 0.0f)
        {
            leftLPF = alpha * leftLPF + (1.0f - alpha) * outL;
            outL = leftLPF;
        }
        else if (pan < 0.0f)
        {
            rightLPF = alpha * rightLPF + (1.0f - alpha) * outR;
            outR = rightLPF;
        }

        left[i]  = outL;
        right[i] = outR;

        delayWriteIndex = (delayWriteIndex + 1) % maxDelaySamples;
    }
}
