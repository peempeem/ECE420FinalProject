#pragma once

#include <opencv2/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "matrix.h"
#include "img_storage.h"

namespace Detection
{
    struct LineData
    {
        float theta;
        float distance;
        float spacing;

        LineData() {}
        LineData(float theta, float distance, float spacing) : theta(theta), distance(distance), spacing(spacing) {}
    };

    void init();
    void getLines(cv::Mat& img, std::vector<LineData>& noteLines, std::vector<LineData>& allLines);
    bool scan(cv::Mat& img, std::vector<LineData>& noteLines, std::vector<Matrix2D<int>>& scans);
    cv::String getNote(std::vector<LineData>& noteLines, int position);
}
