#include "image.h"
#include "../audio/audio.h"
#include "../util/log.h"
#include <opencv2/imgcodecs.hpp>
#include <opencv2/calib3d.hpp>

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


void ImageAnalysis::processImage(cv::Mat& img)
{
    static cv::Mat undistorted;
    static cv::Mat gray;
    static cv::Mat canny;
    static cv::Mat blur;

    auto start = std::chrono::high_resolution_clock::now();

    /////////////////////////////////////////
    //////// Start of Implementation ////////
    /////////////////////////////////////////

    bool useUndistort;
    calibMtx.lock();
    if (calibCalibration)
    {
        cv::undistort(img, undistorted, calibCameraMatrix, calibDistanceCoefficients);
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

    cv::adaptiveThreshold(gray, canny, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY_INV, 3, 3);
    //cv::Canny(gray, canny, 50, 150);

<<<<<<< Updated upstream


    std::vector<std::vector<float>> rtablenote(360, std::vector<float>(2));
    std::vector<std::vector<float>> rtabletreble(360, std::vector<float>(2));
    std::vector<std::vector<float>> rtablebass(360, std::vector<float>(2));
    std::vector<std::vector<float>> rtablesharp(360, std::vector<float>(2));
    std::vector<std::vector<float>> rtableflat(360, std::vector<float>(2));

    std::vector<std::vector<std::vector<float>>> scanned(img.rows,std::vector<std::vector<float>>(img.cols,std::vector<float>(5)));

    make_rtables(rtablenote, rtabletreble,rtablebass,rtablesharp,rtableflat);
    scan(img,scanned,rtablenote,rtabletreble,rtablebass,rtablesharp,rtableflat,10);


    cv::cvtColor(img, gray, cv::COLOR_RGBA2GRAY);
//    cv::adaptiveThreshold(gray, canny,
//                         255, cv::ADAPTIVE_THRESH_GAUSSIAN_C,
//                         cv::THRESH_BINARY, 99, 2);
    cv::Canny(gray, canny, 100, 300);
    std::vector<LineData> lines;
    getLines(canny, lines);
    cv::cvtColor(canny, img, cv::COLOR_GRAY2RGBA);
=======
    std::vector<LineData> noteLines;
    std::vector<LineData> otherLines;
    getLines(canny, noteLines, otherLines);
    //cv::cvtColor(canny, img, cv::COLOR_GRAY2RGBA);

    if (useUndistort)
        undistorted.copyTo(img);
>>>>>>> Stashed changes

    /////////////////////////////////////////
    ///////// End of Implementation /////////
    /////////////////////////////////////////

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    LOGD(TAG, "Processing Time: %lld", duration.count());

    for (LineData& line : otherLines)
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
                 cv::Scalar(0, 255, 0, 255),
                 1);
    }

    for (LineData& line : noteLines)
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
                 cv::Scalar(255, 0, 255, 255),
                 1);

    }
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

    LOGD(TAG, "Calibration Frames: %lu", calibImgPoints.size());
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

void ImageAnalysis::make_rtables(std::vector<std::vector<float>>& rtablenote,
                               std::vector<std::vector<float>>& rtabletreble,
                               std::vector<std::vector<float>>& rtablebass,
                               std::vector<std::vector<float>>& rtablesharp,
                               std::vector<std::vector<float>>& rtableflat)
{
    cv::Mat notemat= cv::Mat::zeros(cv::Size(24,32), CV_64FC1);
    cv::Mat treblemat=cv::Mat::zeros(cv::Size(87,40), CV_64FC1);
    cv::Mat bassmat=cv::Mat::zeros(cv::Size(45,36), CV_64FC1);
    cv::Mat sharpmat=cv::Mat::zeros(cv::Size(36,20), CV_64FC1);
    cv::Mat flatmat=cv::Mat::zeros(cv::Size(36,20), CV_64FC1);
    int threshold = 10;

    for(int i = 0; i<24; i++){
        for(int j=0;j<32;j++){
            notemat.at<int>(i,j)=noteobj[i][j];
        }
    }

    for(int i = 0; i<87; i++){
        for(int j=0;j<40;j++){
            treblemat.at<int>(i,j)=treble[i][j];
        }
    }

    for(int i = 0; i<45; i++){
        for(int j=0;j<36;j++){
            bassmat.at<int>(i,j)=bass[i][j];
        }
    }

    for(int i = 0; i<36; i++){
        for(int j=0;j<20;j++){
            sharpmat.at<int>(i,j)=sharp[i][j];
        }
    }

    for(int i = 0; i<36; i++){
        for(int j=0;j<20;j++){
            flatmat.at<int>(i,j)=flat[i][j];
        }
    }
    train(notemat,threshold,rtablenote);
    train(treblemat,threshold,rtabletreble);
    train(bassmat,threshold,rtablebass);
    train(sharpmat,threshold,rtablesharp);
    train(flatmat,threshold,rtableflat);
}

void ImageAnalysis::audioStats(cv::Mat& img)
{
    std::vector<float> fft;
    std::vector<float> autoCorr;

    AudioAnalyzer::getFFT(fft);
    AudioAnalyzer::getAutoCorrelation(autoCorr);

    // TODO: display fft and autocorr in image
}