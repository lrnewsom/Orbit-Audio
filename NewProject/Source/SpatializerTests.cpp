#include <JuceHeader.h>
#include "Spatializer.h"

//==============================================================================
// I test the Spatializer: at pan=0 I expect equal L/R; at pan=-1 left dominant;
// at pan=+1 right dominant. I run when the user clicks "Run tests" (Audio category).
class SpatializerTest : public juce::UnitTest
{
public:
    SpatializerTest() : juce::UnitTest ("Spatializer", "Audio") {}

    void runTest() override
    {
        Spatializer spatializer;
        const double sampleRate = 44100.0;
        const int blockSize = 256;
        spatializer.prepareToPlay (blockSize, sampleRate);

        juce::AudioBuffer<float> buffer (2, blockSize);

        beginTest ("pan = 0: left and right output equal");
        {
            // I feed a constant 1.0 on both channels and expect equal output (no accidental delay).
            buffer.clear();
            for (int i = 0; i < blockSize; ++i)
            {
                buffer.setSample (0, i, 1.0f);
                buffer.setSample (1, i, 1.0f);
            }
            spatializer.process (buffer, 0, blockSize, 0.0f, Spatializer::OrbitMode::Manual, 0.05f);

            float sumL = 0.0f, sumR = 0.0f;
            for (int i = 0; i < blockSize; ++i)
            {
                sumL += buffer.getSample (0, i);
                sumR += buffer.getSample (1, i);
            }
            sumL /= blockSize;
            sumR /= blockSize;
            expectWithinAbsoluteError (sumL, sumR, 0.01f, "at pan=0 L and R should be equal");
        }

        beginTest ("pan = -1: left dominant over right");
        {
            // I pan full left and expect the left channel to be louder than the right.
            buffer.clear();
            for (int i = 0; i < blockSize; ++i)
            {
                buffer.setSample (0, i, 1.0f);
                buffer.setSample (1, i, 1.0f);
            }
            spatializer.process (buffer, 0, blockSize, -1.0f, Spatializer::OrbitMode::Manual, 0.05f);

            float sumL = 0.0f, sumR = 0.0f;
            for (int i = 0; i < blockSize; ++i)
            {
                sumL += buffer.getSample (0, i);
                sumR += buffer.getSample (1, i);
            }
            sumL /= blockSize;
            sumR /= blockSize;
            expectGreaterThan (sumL, sumR, "at pan=-1 (full left) L should be greater than R");
        }

        beginTest ("pan = +1: right dominant over left");
        {
            // I pan full right and expect the right channel to be louder than the left.
            buffer.clear();
            for (int i = 0; i < blockSize; ++i)
            {
                buffer.setSample (0, i, 1.0f);
                buffer.setSample (1, i, 1.0f);
            }
            spatializer.process (buffer, 0, blockSize, 1.0f, Spatializer::OrbitMode::Manual, 0.05f);

            float sumL = 0.0f, sumR = 0.0f;
            for (int i = 0; i < blockSize; ++i)
            {
                sumL += buffer.getSample (0, i);
                sumR += buffer.getSample (1, i);
            }
            sumL /= blockSize;
            sumR /= blockSize;
            expectGreaterThan (sumR, sumL, "at pan=+1 (full right) R should be greater than L");
        }
    }
};

static SpatializerTest spatializerTest;
