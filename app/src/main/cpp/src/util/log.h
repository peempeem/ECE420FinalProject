#pragma once

#include <android/log.h>

const static char* TAG = "C++";
#define LOGD(tag, message, ...) ((void)__android_log_print(ANDROID_LOG_DEBUG, tag, message, ##__VA_ARGS__))
#define LOGE(tag, message, ...) ((void)__android_log_print(ANDROID_LOG_ERROR, tag, message, ##__VA_ARGS__))