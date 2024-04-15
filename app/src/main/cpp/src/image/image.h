#pragma once

#include "detection.h"

namespace ImageAnalysis
{
    void processImage(cv::Mat& img);
    void accumulator(cv::Mat& img);
    void audioStats(cv::Mat& img);
};
