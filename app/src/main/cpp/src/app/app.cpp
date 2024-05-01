#include "app.h"
#include "../util/log.h"
#include "../util/rate.h"
#include "../audio/audio.h"

static bool newAlloc = true;
static cv::Mat draw;
static Rate startupAnimation;
static Rate currentNoteAnimation(1.5);
static int currentLine;
static int currentNote;
static bool validMusicLines;
static Rate badStartA;
static Rate badStartE;

void restartApp()
{
    startupAnimation.setHertz(1);
    currentLine = -1;
    badStartA.setMs(600);
    badStartE.setMs(300);
    newAlloc = true;
}

bool nextNote(std::vector<Detection::Music>& musicLines)
{
    if (currentLine == -1)
    {
        currentLine = 0;
        currentNote = -1;
    }

    if (currentLine >= musicLines.size())
    {
        currentLine = -1;
        return false;
    }

    if (musicLines[currentLine].clef == Detection::Music::Unknown)
    {
        ++currentLine;
        return nextNote(musicLines);
    }

    ++currentNote;
    if (currentNote >= musicLines[currentLine].notes.size())
    {
        ++currentLine;
        currentNote = -1;
        return nextNote(musicLines);
    }
    return true;
}

void overlayImage(cv::Mat& src, cv::Mat& overlay)
{
    for (unsigned y = 0; y < src.rows; ++y)
    {
        for (unsigned x = 0; x < src.cols; ++x)
        {
            cv::Vec4b& opx = overlay.at<cv::Vec4b>(y, x);
            if (!opx[3])
                continue;

            cv::Vec4b& spx = src.at<cv::Vec4b>(y, x);

            for (unsigned c = 0; c < 3; ++c)
                spx[c] += opx[3] * ((int) opx[c] - (int) spx[c]) / 255;
        }
    }
}

void stepApp(cv::Mat& img)
{
//    auto start = std::chrono::high_resolution_clock::now();
//    LOGD(TAG, "%f", startupAnimation.getStageSin());

    ImageAnalysis::getCapture().copyTo(img);
    auto& musicLines = ImageAnalysis::getMusicLines();

    if (newAlloc)
    {
        draw = cv::Mat::zeros(img.rows, img.cols, CV_8UC4);
        nextNote(musicLines);

        if (currentLine != -1)
        {
            AudioAnalyzer::playNote(musicLines[currentLine].notes[currentNote].data);
            validMusicLines = true;
        }
        else
            validMusicLines = false;
        newAlloc = false;
    }
    else
    {
        for (unsigned y = 0; y < draw.rows; ++y)
        {
            for (unsigned x = 0; x < draw.cols; ++x)
                draw.at<cv::Vec4b>(y, x)[3] = 0;
        }
    }

    if (!validMusicLines)
    {
        if (badStartA.isReady())
        {
            badStartA.disable();
            AudioAnalyzer::playNote(musicNote.fromName("A2"));
        }

        if (badStartE.isReady())
        {
            badStartE.disable();
            AudioAnalyzer::playNote(musicNote.fromName("E3"));
        }
    }

    for (auto& musicLine : musicLines)
    {
        float a = cosf(musicLine.angle);
        float b = sinf(musicLine.angle);

        for (unsigned i = 0; i < 5; ++i)
        {
            int x0 = a * musicLine.distance[i];
            int y0 = b * musicLine.distance[i];

            int x1 = x0 + (int) 10000 * (-b);
            int y1 = y0 + (int) 10000 * a;

            int x2 = x0 - (int) 10000 * (-b);
            int y2 = y0 - (int) 10000 * a;

            cv::line(draw,
                     cv::Point(x1, y1),
                     cv::Point(x2, y2),
                     cv::Scalar(255, 0, 255, 255 * startupAnimation.getStageRamp()),
                     1);
        }
    }

    for (auto& line : musicLines)
    {
        if (line.clef == Detection::Music::Treble)
        {
            cv::circle(draw,
                       cv::Point(line.clefPos.x, line.clefPos.y),
                       20,
                       cv::Scalar(255, 255, 0, 255 * startupAnimation.getStageRamp()));
        }
        else if (line.clef == Detection::Music::Bass)
        {
            cv::circle(draw,
                       cv::Point(line.clefPos.x, line.clefPos.y),
                       20,
                       cv::Scalar(0, 255, 255, 255 * startupAnimation.getStageRamp()));
        }

        for (auto& note : line.notes)
        {
            if (note.data)
            {
                cv::Size textSize = cv::getTextSize(
                        note.data->name,
                        cv::FONT_HERSHEY_SIMPLEX,
                        musicLines.front().spacing / 15.0f,
                        2,
                        NULL);

                cv::putText(draw,
                            note.data->name,
                            cv::Point(note.position.x - textSize.width / 2,
                                      note.position.y - 50 * (musicLines.front().spacing / 15.0f) + textSize.height / 2),
                            cv::FONT_HERSHEY_SIMPLEX,
                            musicLines.front().spacing / 15.0f,
                            cv::Scalar(255,0,0,255 * startupAnimation.getStageRamp()),
                            2);
            }
        }
    }

    if (currentLine != -1)
    {
        cv::circle(draw,
                   cv::Point(musicLines[currentLine].notes[currentNote].position.x,
                             musicLines[currentLine].notes[currentNote].position.y),
                   30 * (musicLines.front().spacing / 15.0f),
                   cv::Scalar(
                           0,
                           255,
                           0,
                           (55 + 200 * currentNoteAnimation.getStageCos(0, true)) * startupAnimation.getStageRamp()),
                   2);

        cv::Size textSize = cv::getTextSize(
                musicLines[currentLine].notes[currentNote].data->name,
                cv::FONT_HERSHEY_SIMPLEX,
                musicLines.front().spacing / 15.0f,
                2,
                NULL);

        cv::putText(draw,
                    musicLines[currentLine].notes[currentNote].data->name,
                    cv::Point(musicLines[currentLine].notes[currentNote].position.x - textSize.width / 2,
                              musicLines[currentLine].notes[currentNote].position.y - 50 * (musicLines.front().spacing / 15.0f) + textSize.height / 2),
                    cv::FONT_HERSHEY_SIMPLEX,
                    musicLines.front().spacing / 15.0f,
                    cv::Scalar(
                            0,
                            255,
                            0,
                            (55 + 200 * currentNoteAnimation.getStageCos(0, true)) * startupAnimation.getStageRamp()),
                    2);
    }

    if (!validMusicLines)
    {
        const char* str = "Nothing to see here :(";
        cv::Size textSize = cv::getTextSize(
                str,
                cv::FONT_HERSHEY_SIMPLEX,
                2,
                3,
                NULL);

        cv::putText(draw,
                    str,
                    cv::Point(draw.cols / 2 - textSize.width / 2,
                              draw.rows / 2 + textSize.height / 2),
                    cv::FONT_HERSHEY_SIMPLEX,
                    2,
                    cv::Scalar(
                            255,
                            0,
                            255,
                            255 * startupAnimation.getStageRamp()),
                    3);
        LOGD(TAG, "%f", startupAnimation.getStageRamp());
    }
    else if (currentLine == -1)
    {
        const char* str = "Nice Work!";
        cv::Size textSize = cv::getTextSize(
                str,
                cv::FONT_HERSHEY_SIMPLEX,
                2,
                3,
                NULL);

        cv::putText(draw,
                    str,
                    cv::Point(draw.cols / 2 - textSize.width / 2,
                              draw.rows - textSize.height),
                    cv::FONT_HERSHEY_SIMPLEX,
                    2,
                    cv::Scalar(
                            255,
                            0,
                            255,
                            255 * startupAnimation.getStageRamp()),
                    2);
    }

    auto* note = AudioAnalyzer::getCurrentNote();
    if (note && currentLine != -1)
    {
        if (note->midi == musicLines[currentLine].notes[currentNote].data->midi)
        {
            nextNote(musicLines);
            if (currentLine != -1)
                AudioAnalyzer::playNote(musicLines[currentLine].notes[currentNote].data);
        }
    }

    overlayImage(img, draw);


//    auto end = std::chrono::high_resolution_clock::now();
//    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
//    LOGD(TAG, "Draw Time: %lld", duration.count());
}