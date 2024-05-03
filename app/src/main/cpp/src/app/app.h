#pragma once

#include "../image/image.h"

void restartApp();
void transpose(int value);
const char* getDetectedKey();
const char* getTransposedKey();
void stepApp(cv::Mat& img);
