#pragma once

#include <opencv2/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>

void get_gradient(cv::Mat& pic, std::vector<std::vector<std::vector<float>>>& grad);
void train(cv::Mat& img, int threshold, std::vector<std::vector<float>>& rtable);
void scan(cv::Mat& img,
          std::vector<std::vector<std::vector<float>>>& grad,
          std::vector<std::vector<float>>& rtablenote,
          std::vector<std::vector<float>>& rtabletreble,
          std::vector<std::vector<float>>& rtablebass,
          std::vector<std::vector<float>>& rtablenotesharp,
          std::vector<std::vector<float>>& rtableflat,
          float spacing);
int ddpeaks(std::vector<std::vector<float>>& array,
             std::vector<std::vector<int>>& peaks,
             int threshold, float spacing, int cols, int rows);