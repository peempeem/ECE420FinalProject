#pragma once

#include "detection.h"

namespace ImageAnalysis
{
    void init();
    void processImage(cv::Mat& img);
    void beginCalibration();
    void calibrateCamera(cv::Mat& img);
    void endCalibration();

    cv::Mat& getCapture();
    std::vector<Detection::Music>& getMusicLines();

}
