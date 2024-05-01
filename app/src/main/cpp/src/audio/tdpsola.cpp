#include "tdpsola.h"
#include <string>
#include "pianoSample.h"

using namespace std;

#define EPOCH_PEAK_REGION_WIGGLE 30

// Global Variables
int F_S;
int newEpochIdx;
int BUFFER_SIZE;

/* Citation: Some codes (including functions) are borrowed from ECE420 Spring 2024 Lab 5*/
int findMaxArrayIdx(float *array, int minIdx, int maxIdx) {
    int ret_idx = minIdx;

    for (int i = minIdx; i < maxIdx; i++) {
        if (array[i] > array[ret_idx]) {
            ret_idx = i;
        }
    }

    return ret_idx;
}

void findEpochLocations(std::vector<int> &epochLocations, float *buffer, int periodLen) {
    int largestPeak = findMaxArrayIdx(buffer, 0, BUFFER_SIZE);
    epochLocations.push_back(largestPeak);

    // First go right
    int epochCandidateIdx = epochLocations[0] + periodLen;
    while (epochCandidateIdx < BUFFER_SIZE) {
        epochLocations.push_back(epochCandidateIdx);
        epochCandidateIdx += periodLen;
    }

    // Then go left
    epochCandidateIdx = epochLocations[0] - periodLen;
    while (epochCandidateIdx > 0) {
        epochLocations.push_back(epochCandidateIdx);
        epochCandidateIdx -= periodLen;
    }

    // Sort in place so that we can more easily find the period,
    // where period = (epochLocations[t+1] + epochLocations[t-1]) / 2
    std::sort(epochLocations.begin(), epochLocations.end());

    // Finally, just to make sure we have our epochs in the right
    // place, ensure that every epoch mark (sans first/last) sits on a peak
    for (int i = 1; i < epochLocations.size() - 1; i++) {
        int minIdx = epochLocations[i] - EPOCH_PEAK_REGION_WIGGLE;
        int maxIdx = epochLocations[i] + EPOCH_PEAK_REGION_WIGGLE;

        int peakOffset = findMaxArrayIdx(buffer, minIdx, maxIdx) - minIdx;
        peakOffset -= EPOCH_PEAK_REGION_WIGGLE;

        epochLocations[i] += peakOffset;
    }
}

void overlapAddArray(float *dest, float *src, int startIdx, int len) {
    int idxLow = startIdx;
    int idxHigh = startIdx + len;

    int padLow = 0;
    int padHigh = 0;
    if (idxLow < 0) {
        padLow = -idxLow;
    }
    if (idxHigh > BUFFER_SIZE) {
        padHigh = BUFFER_SIZE - idxHigh;
    }

    // Finally, reconstruct the buffer
    for (int i = padLow; i < len + padHigh; i++) {
        dest[startIdx + i] += src[i];
    }
}

int findClosestInVector(std::vector<int> vec, float value, int minIdx, int maxIdx) {
    int retIdx = minIdx;
    float bestResid = abs(vec[retIdx] - value);

    for (int i = minIdx; i < maxIdx; i++) {
        if (abs(vec[i] - value) < bestResid) {
            bestResid = abs(vec[i] - value);
            retIdx = i;
        }
    }

    return retIdx;
}

// https://en.wikipedia.org/wiki/Hann_function
float getHanningCoef(int N, int idx) {
    return (float) (0.5 * (1.0 - cos(2.0 * M_PI * idx / (N - 1))));
}

void tdpsola(int FREQ_NEW) {
//    /* AudioFile.h borrowed from https://github.com/adamstark/AudioFile */
//    AudioFile<double> audioFile;
//    audioFile.load("piano-C4.wav"); /* sample piano-C4 downloaded from https://www.ee.columbia.edu/~dpwe/sounds/instruments/ */
//
//    int channel = 0;
//    int numSamples = audioFile.getNumSamplesPerChannel(); // eg. 29750
//
//    BUFFER_SIZE = numSamples;
//    F_S = audioFile.getSampleRate(); // eg. 11025
//
//    float sampleNote[BUFFER_SIZE] = {};
//    float desiredNote[BUFFER_SIZE] = {};
//    for (int i = 0; i < numSamples; i++)
//    {
//        sampleNote[i] = (float) (audioFile.samples[channel][i]);
//    }
//
//    std::vector<int> epochLocations;
//    findEpochLocations(epochLocations, sampleNote, F_S/261);
//
//    int P_1 = F_S/FREQ_NEW; // P1, new_epoch_spacing, eg. 28
//
//    for (int i = 0; i < numSamples; i += P_1) {
//        int closest_epoch_index = findClosestInVector(epochLocations, i, 1, epochLocations.size()-1); // Find closest epoch of current new epoch from (original) audio_data
//        int P_0 = (epochLocations[closest_epoch_index + 1] - epochLocations[closest_epoch_index - 1]) / 2; // Find P_0
//
//        // Compute impulse response with Hamming window, with the size of 2*P_0+1
//        int index = 0;
//        float overlapAddBuffer[BUFFER_SIZE] = {};
//        for (int j = epochLocations[closest_epoch_index]-P_0; j < epochLocations[closest_epoch_index]+P_0+1; j++) {
//            overlapAddBuffer[index] = sampleNote[j] * getHanningCoef(2*P_0+1, index);
//            index += 1;
//        }
//
//        overlapAddArray(desiredNote, overlapAddBuffer, i-P_0, 2*P_0+1);
//    }
//
//    AudioFile<double>::AudioBuffer buffer;
//    buffer.resize(1);
//    buffer[0].resize(BUFFER_SIZE);
//    for (int i = 0; i < BUFFER_SIZE; i++) {
//        buffer[0][i] = desiredNote[i];
//    }
//    bool ok = audioFile.setAudioBuffer(buffer);
//    string name = "pianoTone";
//    string newToneHz = name.append(std::to_string(FREQ_NEW));
//    string fileName = newToneHz.append(".wav");
//    audioFile.save(fileName);
//    std::cout << "Saving (TD-PSOLA) target -> " << fileName << " <- completed ... !";
}
