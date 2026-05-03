#include "AutoLevel.h"

void AutoLevel::prepare(double sampleRate)
{
    fs = sampleRate;
    const int maxSamples = static_cast<int>(std::ceil(192000.0 * 10.0));
    dryHistoryBuffer.assign(maxSamples, 0.0f);
    wetHistoryBuffer.assign(maxSamples, 0.0f);
    dryWriteIndex = 0;
    wetWriteIndex = 0;

    latestDryRms.store(0.0f);
    latestWetRms.store(0.0f);
}

void AutoLevel::pushDrySample(float input)
{
    dryHistoryBuffer[dryWriteIndex] = input;
    dryWriteIndex = (dryWriteIndex + 1) % dryHistoryBuffer.size();
}

void AutoLevel::pushWetSample(float input)
{
    wetHistoryBuffer[wetWriteIndex] = input;
    wetWriteIndex = (wetWriteIndex + 1) % wetHistoryBuffer.size();
}

void AutoLevel::analyzeRMS(float seconds)
{
    int numSamples = static_cast<int>(fs * seconds);
    int bufferSize = static_cast<int>(dryHistoryBuffer.size());

    if (numSamples > bufferSize) numSamples = bufferSize;
    if (numSamples <= 0) {
        latestDryRms.store(0.0f);
        latestWetRms.store(0.0f);
        return;
    }

    double sumSquaresDry = 0.0;
    double sumSquaresWet = 0.0;

    int readIdxDry = dryWriteIndex - 1;
    int readIdxWet = wetWriteIndex - 1;

    for (int i = 0; i < numSamples; ++i)
    {
        if (readIdxDry < 0) readIdxDry += bufferSize;
        if (readIdxWet < 0) readIdxWet += bufferSize;

        float dVal = dryHistoryBuffer[readIdxDry];
        float wVal = wetHistoryBuffer[readIdxWet];

        sumSquaresDry += static_cast<double>(dVal * dVal);
        sumSquaresWet += static_cast<double>(wVal * wVal);

        readIdxDry--;
        readIdxWet--;
    }

    latestDryRms.store(static_cast<float>(std::sqrt(sumSquaresDry / numSamples)));
    latestWetRms.store(static_cast<float>(std::sqrt(sumSquaresWet / numSamples)));
}