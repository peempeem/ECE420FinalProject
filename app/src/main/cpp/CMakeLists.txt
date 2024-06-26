cmake_minimum_required(VERSION 3.22.1)


project("ece420finalproject")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -Wall -Werror")

include_directories(src/main/cpp/include/)

add_library(${CMAKE_PROJECT_NAME} SHARED
        native-lib.cpp
        src/image/image.h
        src/image/image.cpp
        src/image/detection.h
        src/image/detection.cpp
        src/image/matrix.h
        src/image/matrix.hpp
        src/audio/audio.h
        src/image/img_storage.h
        src/image/img_storage.cpp
        src/audio/audio.h
        src/audio/audio.cpp
        src/audio/note.h
        src/audio/note.cpp
        src/audio/tdpsola.h
        src/audio/tdpsola.cpp
        src/app/app.h
        src/app/app.cpp
        src/util/log.h
        src/util/rate.h
        src/util/rate.cpp
        src/util/stream.h
        src/util/stream.hpp
        kiss_fft/kiss_fft.c
        kiss_fft/kiss_fft.h)

file(GLOB_RECURSE SOURCES "./src/**.cpp")

set(OpenCV_DIR "../../../../sdk/native/jni")
find_package(OpenCV REQUIRED)

target_link_libraries(
        ${CMAKE_PROJECT_NAME} -ljnigraphics
        android
        log
        aaudio
        ${OpenCV_LIBS})
