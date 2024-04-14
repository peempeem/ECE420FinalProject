cmake_minimum_required(VERSION 3.22.1)

project("ece420finalproject")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror")

include_directories(src/main/cpp/include/)

add_library(${CMAKE_PROJECT_NAME} SHARED
        native-lib.cpp
        src/image/image.cpp
        src/audio/audio.cpp
        src/audio/note.cpp
        kiss_fft/kiss_fft.c)

file(GLOB_RECURSE SOURCES "./src/**.cpp")

set(OpenCV_DIR "../../../../sdk/native/jni")
find_package(OpenCV REQUIRED)

target_link_libraries(
        ${CMAKE_PROJECT_NAME} -ljnigraphics
        android
        log
        aaudio
        ${OpenCV_LIBS})
