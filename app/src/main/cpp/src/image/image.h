#pragma once

#include "detection.h"

namespace ImageAnalysis
{
    void processImage(cv::Mat& img);
    void accumulator(cv::Mat& img);
    void beginCalibration();
    void calibrateCamera(cv::Mat& img);
    void endCalibration();
    void audioStats(cv::Mat& img);
};
