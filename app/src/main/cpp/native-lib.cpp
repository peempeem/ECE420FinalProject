#include <jni.h>
#include <android/bitmap.h>
#include <aaudio/AAudio.h>
#include <chrono>

#include "src/util/log.h"
#include "src/image/image.h"
#include "src/audio/audio.h"
#include "src/app/app.h"

extern "C"
JNIEXPORT void JNICALL
Java_com_example_ece420finalproject_MainActivity_initCPP(JNIEnv* env, jobject ob)
{
    AudioAnalyzer::init();
    ImageAnalysis::init();
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
    ImageAnalysis::processImage(img);
    AndroidBitmap_unlockPixels(env, bitmap);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_ece420finalproject_MainActivity_beginCalibration(JNIEnv* env, jobject ob)
{
    ImageAnalysis::beginCalibration();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_ece420finalproject_MainActivity_calibrationStep(JNIEnv* env, jobject ob, jobject bitmap)
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
    ImageAnalysis::calibrateCamera(img);
    AndroidBitmap_unlockPixels(env, bitmap);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_ece420finalproject_MainActivity_endCalibration(JNIEnv* env, jobject ob)
{
    ImageAnalysis::endCalibration();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_ece420finalproject_MainActivity_restartPlayback(JNIEnv* env, jobject ob)
{
    restartApp();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_ece420finalproject_MainActivity_stepPlayback(JNIEnv* env, jobject ob, jobject bitmap)
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
    stepApp(img);
    AndroidBitmap_unlockPixels(env, bitmap);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_ece420finalproject_MainActivity_transpose(JNIEnv* env, jobject ob, jint transNumber)
{
    transpose(transNumber);
}
