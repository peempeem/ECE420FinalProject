#include <jni.h>
#include <android/bitmap.h>


#include <opencv2/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <aaudio/AAudio.h>

#include <chrono>

#include "src/util/log.h"
#include "src/audio/audio.h"


void processImage(cv::Mat& img)
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

extern "C"
JNIEXPORT void JNICALL
Java_com_example_ece420finalproject_MainActivity_initCPP(JNIEnv* env, jobject ob)
{
    AudioAnalyzer::init();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_ece420finalproject_MainActivity_pauseCPP(JNIEnv* env, jobject ob)
{
    AudioAnalyzer::pause();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_ece420finalproject_MainActivity_resumeCPP(JNIEnv* env, jobject ob)
{
    AudioAnalyzer::resume();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_ece420finalproject_MainActivity_endCPP(JNIEnv* env, jobject ob)
{
    AudioAnalyzer::deinit();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_ece420finalproject_MainActivity_processImage(JNIEnv* env, jobject ob, jobject bitmap)
{
    AndroidBitmapInfo info;
    AndroidBitmap_getInfo(env, bitmap, &info);

    if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888)
    {
        LOGD(TAG, "Bitmap format is not RGBA_8888!");
        return;
    }

    void* pixels;
    AndroidBitmap_lockPixels(env, bitmap, &pixels);
    cv::Mat img(info.height, info.width, CV_8UC4, pixels);
    processImage(img);
    AndroidBitmap_unlockPixels(env, bitmap);
}
