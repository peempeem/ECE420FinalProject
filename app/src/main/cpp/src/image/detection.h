#pragma once

#include <opencv2/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "matrix.h"
#include "img_storage.h"
#include "../audio/note.h"

namespace Detection
{
    struct Linef
    {
        float m;
        float b;
    };

    struct Pointf
    {
        float x;
        float y;

        Pointf() {}
        Pointf(float x, float y) : x(x), y(y) {}
    };

    struct LineData
    {
        float theta;
        float distance;
        float spacing;

        LineData() {}
        LineData(float theta, float distance, float spacing) : theta(theta), distance(distance), spacing(spacing) {}
    };

    struct Music
    {
        enum Clef
        {
            Unknown,
            Treble,
            Bass
        };

        struct Note
        {
            MusicNote::Data data;
            Pointf position;
        };

        Clef clef = Unknown;
        Pointf clefPos;
        float spacing;
        float angle;
        float middle;
        float distance[5];
        std::vector<Note> notes;
    };

    void init();
    void getMusicLines(cv::Mat& img, std::vector<Music>& musicLines, std::vector<LineData>& allLines);
    cv::Mat scan(cv::Mat& img, std::vector<Music>& musicLines);
    cv::String getNote(std::vector<LineData>& noteLines, int position);
}
