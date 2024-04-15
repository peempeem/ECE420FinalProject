#include "image.h"
#include "../audio/audio.h"

void ImageAnalysis::processImage(cv::Mat& img)
{
    //auto start = std::chrono::high_resolution_clock::now();

    /////////////////////////////////////////
    //////// Start of Implementation ////////
    /////////////////////////////////////////

    static cv::Mat gray;
    static cv::Mat edges;
    static cv::Mat edges3;

    cv::cvtColor(img, gray, cv::COLOR_BGRA2GRAY);
    cv::Canny(gray, edges, 100, 10000, 7, true);
    cv::cvtColor(edges, edges3, cv::COLOR_GRAY2BGRA);
    img += edges3;

    /////////////////////////////////////////
    ///////// End of Implementation /////////
    /////////////////////////////////////////

    //auto end = std::chrono::high_resolution_clock::now();
    //auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    //LOGD(TAG, "Processing Time: %lld", duration.count());
}

void ImageAnalysis::audioStats(cv::Mat& img)
{
    std::vector<float> fft;
    std::vector<float> autoCorr;

    AudioAnalyzer::getFFT(fft);
    AudioAnalyzer::getAutoCorrelation(autoCorr);

    // TODO: display fft and autocorr in image
}