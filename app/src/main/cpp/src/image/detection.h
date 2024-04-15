#pragma once

#include <opencv2/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>

void get_gradient(cv::Mat& pic, std::vector<std::vector<std::vector<float>>>& grad);
void train(cv::Mat& img, int threshold, std::vector<std::vector<float>>& rtable);