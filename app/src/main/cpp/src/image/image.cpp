#include "image.h"
#include "../audio/audio.h"
#include "../util/log.h"
#include <opencv2/imgcodecs.hpp>
#include <opencv2/calib3d.hpp>

///////////////////////////////////////
//////// Calibration Variables ////////
///////////////////////////////////////

std::vector<std::vector<cv::Point2f>> calibImgPoints;
cv::Mat calibCameraMatrix;
cv::Mat calibDistanceCoefficients;
cv::Mat calibR;
cv::Mat calibT;
unsigned calibRows = -1;
unsigned calibCols = -1;
unsigned calibFrames = 0;
bool calibCalibration = false;
std::mutex calibMtx;

#define CHECKERBOARD_X 7
#define CHECKERBOARD_Y 7

void ImageAnalysis::init()
{
    Detection::init();
}

void drawLines(cv::Mat img, std::vector<Detection::LineData> lines, cv::Scalar color)
{
    for (auto& line : lines)
    {
        float a = cosf(line.theta);
        float b = sinf(line.theta);

        int x0 = a * line.distance;
        int y0 = b * line.distance;

        int x1 = x0 + (int) 10000 * (-b);
        int y1 = y0 + (int) 10000 * a;

        int x2 = x0 - (int) 10000 * (-b);
        int y2 = y0 - (int) 10000 * a;

        cv::line(img,
                 cv::Point(x1, y1),
                 cv::Point(x2, y2),
                 color,
                 1);
    }
}

void ImageAnalysis::processImage(cv::Mat& img)
{
    static cv::Mat undistorted;
    static cv::Mat gray;
    static cv::Mat adapt;
    static std::vector<Matrix2D<int>> scans;

    auto start = std::chrono::high_resolution_clock::now();

    /////////////////////////////////////////
    //////// Start of Implementation ////////
    /////////////////////////////////////////

    bool useUndistort;
    calibMtx.lock();
    if (calibCalibration)
    {
        cv::undistort(
                img,
                undistorted,
                calibCameraMatrix,
                calibDistanceCoefficients);

        useUndistort = true;
        calibMtx.unlock();
        cv::cvtColor(undistorted, gray, cv::COLOR_RGBA2GRAY);
    }
    else
    {
        useUndistort = false;
        calibMtx.unlock();
        cv::cvtColor(img, gray, cv::COLOR_RGBA2GRAY);
    }

    cv::adaptiveThreshold(
            gray,
            adapt,
            255,
            cv::ADAPTIVE_THRESH_GAUSSIAN_C,
            cv::THRESH_BINARY_INV,
            3,
            3);

    std::vector<Detection::LineData> noteLines;
    std::vector<Detection::LineData> allLines;
    getLines(adapt, noteLines, allLines);

    bool scanned = Detection::scan(gray, noteLines, scans);

    //cv::cvtColor(adapt, img, cv::COLOR_GRAY2RGBA);

    /////////////////////////////////////////
    ///////// End of Implementation /////////
    /////////////////////////////////////////

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    LOGD(TAG, "Processing Time: %lld", duration.count());

    start = std::chrono::high_resolution_clock::now();

    if (useUndistort)
        undistorted.copyTo(img);

    if (scanned)
    {
        for (auto& peak : scans[0].peaks(5, 8, 8))
        {
            cv::circle(img, cv::Point(peak.point.x, peak.point.y), 20, cv::Scalar(255, 0, 255, 255));
            LOGD(TAG, "%d", peak.energy);
        }
    }


    drawLines(img, allLines, cv::Scalar(0, 255, 0, 255));
    drawLines(img, noteLines, cv::Scalar(255, 0, 255, 255));

    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    LOGD(TAG, "Draw Time: %lld", duration.count());
}

void ImageAnalysis::accumulator(cv::Mat& img)
{

}

void ImageAnalysis::beginCalibration()
{
    calibMtx.lock();
    calibImgPoints.clear();
    calibFrames = 0;
    calibCalibration = false;
    calibMtx.unlock();
}

void ImageAnalysis::calibrateCamera(cv::Mat& img)
{
    cv::Mat gray;
    cv::cvtColor(img, gray, cv::COLOR_RGBA2GRAY);

    std::vector<cv::Point2f> imgPts;
    if (cv::findChessboardCorners(
            gray,
            cv::Size(7, 7),
            imgPts,
            cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_FAST_CHECK | cv::CALIB_CB_NORMALIZE_IMAGE))
    {
        if (calibFrames++ % 20 == 0)
        {
            calibMtx.lock();
            calibRows = img.rows;
            calibCols = img.cols;
            calibImgPoints.push_back(imgPts);
            calibMtx.unlock();
        }

        cv::drawChessboardCorners(img, cv::Size(CHECKERBOARD_X,CHECKERBOARD_Y), imgPts, true);
    }

}

void ImageAnalysis::endCalibration() {
    calibMtx.lock();
    std::vector<cv::Point3f> wldPts;
    for (unsigned y = 0; y < CHECKERBOARD_Y; ++y) {
        for (unsigned x = 0; x < CHECKERBOARD_X; ++x)
            wldPts.emplace_back(x, y, 0);
    }

    std::vector<std::vector<cv::Point3f>> objPts;
    for (unsigned i = 0; i < calibImgPoints.size(); ++i)
        objPts.push_back(wldPts);

    LOGD(TAG, "Calibration Frames: %u", (unsigned) calibImgPoints.size());
    if (!calibImgPoints.empty()) {
        cv::calibrateCamera(
                objPts,
                calibImgPoints,
                cv::Size(calibRows, calibCols),
                calibCameraMatrix,
                calibDistanceCoefficients,
                calibR,
                calibT);
        calibCalibration = true;
    }
    calibMtx.unlock();
    LOGD(TAG, "Camera Calibration Complete");
}

void ImageAnalysis::audioStats(cv::Mat& img)
{
    std::vector<float> fft;
    std::vector<float> autoCorr;

    AudioAnalyzer::getFFT(fft);
    AudioAnalyzer::getAutoCorrelation(autoCorr);

    // TODO: display fft and autocorr in image
}