#pragma once

#include "detection.h"
#include "img_storage.h"

namespace ImageAnalysis
{
    void processImage(cv::Mat& img);
    void accumulator(cv::Mat& img);
    void audioStats(cv::Mat& img);
    void make_rtables(std::vector<std::vector<float>>& rtablenote,
                      std::vector<std::vector<float>>& rtabletreble,
                      std::vector<std::vector<float>>& rtablebass,
                      std::vector<std::vector<float>>& rtablesharp,
                      std::vector<std::vector<float>>& rtableflat);
};
