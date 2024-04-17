#include "image.h"
#include "../audio/audio.h"
#include "../util/log.h"

void ImageAnalysis::processImage(cv::Mat& img)
{
    static cv::Mat gray;
    static cv::Mat canny;

    auto start = std::chrono::high_resolution_clock::now();

    /////////////////////////////////////////
    //////// Start of Implementation ////////
    /////////////////////////////////////////

    cv::cvtColor(img, gray, cv::COLOR_RGBA2GRAY);
//    cv::adaptiveThreshold(gray, canny,
//                         255, cv::ADAPTIVE_THRESH_GAUSSIAN_C,
//                         cv::THRESH_BINARY, 99, 2);
    cv::Canny(gray, canny, 100, 300);
    std::vector<LineData> lines;
    getLines(canny, lines);
    cv::cvtColor(canny, img, cv::COLOR_GRAY2RGBA);

    /////////////////////////////////////////
    ///////// End of Implementation /////////
    /////////////////////////////////////////

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    LOGD(TAG, "Processing Time: %lld", duration.count());

    for (LineData& line : lines)
    {
        float a = cosf(line.theta / 100.0f);
        float b = sinf(line.theta / 100.0f);

        int x0 = a * line.distance;
        int y0 = b * line.distance;

        int x1 = x0 + 1000 * (-b);
        int y1 = y0 + 1000 * a;

        int x2 = x0 - 1000 * (-b);
        int y2 = y0 - 1000 * a;

        cv::line(img,
                 cv::Point(x1, y1),
                 cv::Point(x2, y2),
                 cv::Scalar(255, 0, 255, 255),
                 1);
    }



}

void ImageAnalysis::accumulator(cv::Mat& img)
{

}

void ImageAnalysis::audioStats(cv::Mat& img)
{
    std::vector<float> fft;
    std::vector<float> autoCorr;

    AudioAnalyzer::getFFT(fft);
    AudioAnalyzer::getAutoCorrelation(autoCorr);

    // TODO: display fft and autocorr in image
}