#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <vector>
#include "../../kiss_fft/kiss_fft.h"

int findMaxArrayIdx(float *array, int minIdx, int maxIdx);
void findEpochLocations(std::vector<int> &epochLocations, float *buffer, int periodLen);
void overlapAddArray(float *dest, float *src, int startIdx, int len);
int findClosestInVector(std::vector<int> vec, float value, int minIdx, int maxIdx);
float getHanningCoef(int N, int idx);
void tdpsola(int FREQ_NEW);
