#include "detection.h"
#include "math.h"
#include "../util/log.h"
#include "img_storage.h"
#include <map>

#define LINE_ACCUMULATION_ANGLES 200
#define OBJ_RTABLE_ANGLES 100
#define RTABLE_THRESHOLD 10

static float cosValues[LINE_ACCUMULATION_ANGLES];
static float sinValues[LINE_ACCUMULATION_ANGLES];

std::vector<std::vector<Float2>> rTables;

void train(Matrix2D<int>& encoding, int threshold)
{
    Matrix2D<Float2> grad;
    encoding.gradient(grad);

    float centerX = encoding.width() / 2.0f;
    float centerY = encoding.height() / 2.0f;

    rTables.emplace_back(OBJ_RTABLE_ANGLES, Float2(0, 0));
    std::vector<Float2>& rTable = rTables.back();

    for (unsigned y = 0; y < encoding.height(); ++y)
    {
        for (unsigned x = 0; x < encoding.width(); ++x)
        {
            Float2& data = grad.at(y, x);
            if (data.magnitude() >= threshold)
            {
                unsigned angle = (unsigned) ((OBJ_RTABLE_ANGLES - 1) * (0.5f + (data.atan2() / (2 * M_PI))));
                rTable.at(angle) = Float2(y - centerY, x - centerX);
            }
        }
    }
}

#define loadRTable(loadEncoding)                                                    \
{                                                                                   \
    Matrix2D<int> encoding(sizeof(loadEncoding) / sizeof(loadEncoding[0]),          \
                           sizeof(loadEncoding[0]) / sizeof(loadEncoding[0][0]),    \
                           (const int *) loadEncoding);                             \
    train(encoding, RTABLE_THRESHOLD);                                              \
}

void Detection::init()
{
    for (unsigned i = 0; i < LINE_ACCUMULATION_ANGLES; ++i)
    {
        cosValues[i] = cosf(M_PI * i / (float) LINE_ACCUMULATION_ANGLES);
        sinValues[i] = sinf(M_PI * i / (float) LINE_ACCUMULATION_ANGLES);
    }

    loadRTable(noteEncoding);
    loadRTable(trebleEncoding);
    loadRTable(bassEncoding);
    loadRTable(sharpEncoding);
    loadRTable(flatEncoding);
}

void Detection::getLines(cv::Mat& img, std::vector<LineData>& noteLines, std::vector<LineData>& allLines) {
    static Matrix2D<int> accumulation;

    auto start = std::chrono::high_resolution_clock::now();

    // diagonal of image
    int rmax = sqrtf(img.rows * img.rows + img.cols * img.cols);
    accumulation.resize(LINE_ACCUMULATION_ANGLES, 2 * rmax);
    accumulation.fill(0);

    // accumulation
    for (unsigned r = 0; r < img.rows; ++r) {
        for (unsigned c = 0; c < img.cols; ++c) {
            if (img.at<uint8_t>(r, c) < 127)
                continue;

            for (unsigned theta = 0; theta < LINE_ACCUMULATION_ANGLES; ++theta) {
                int dist = roundf(c * cosValues[theta] + r * sinValues[theta]);
                if (dist >= -rmax && dist < rmax)
                    accumulation.at(theta, dist + rmax)++;
            }
        }
    }

    auto rawPeaks = accumulation.peaks(250, 16, 8);

    for (auto& peak : rawPeaks)
        allLines.emplace_back(
                M_PI * peak.point.y / (float) LINE_ACCUMULATION_ANGLES,
                peak.point.x - rmax,
                0);

    struct PointPeak
    {
        Matrix2D<int>::Peak peak;
        int yMid;

        PointPeak() {}
        PointPeak(Matrix2D<int>::Peak& peak, int yMid) : peak(peak), yMid(yMid) {}

        bool operator<(const PointPeak& other) const
        {
            return yMid < other.yMid;
        }
    };
    std::vector<PointPeak> pointPeaks;

    for (unsigned i = 0; i < rawPeaks.size(); ++i)
    {
        float a = cosValues[rawPeaks[i].point.y];
        float b = sinValues[rawPeaks[i].point.y];

        int dist = (int) rawPeaks[i].point.x - rmax;
        float x0 = a * dist;
        float y0 = b * dist;
        int xMid = img.cols / 2.0f;
        int yMid = -(a / b) * (xMid - x0) + y0;

        if (yMid < 0 || yMid > img.rows)
            continue;

        pointPeaks.emplace_back(rawPeaks[i], yMid);
    }

    if (pointPeaks.size() >= 5)
    {
        std::sort(pointPeaks.begin(), pointPeaks.end());

        std::vector<unsigned> pointDiff(pointPeaks.size() - 1);
        for (unsigned i = 0; i < pointDiff.size(); ++i)
            pointDiff[i] = pointPeaks[i + 1].yMid - pointPeaks[i].yMid;

        for (unsigned i = 0; i < pointPeaks.size(); ++i)
        {
            if (i > pointPeaks.size() - 5)
                break;

            float cmp1 = pointDiff[i];
            bool found = true;


            for (unsigned j = 0; j < 4; ++j)
            {
                float cmp2 = pointDiff[i + j];
                float cmp = cmp2 / cmp1;
                if (cmp < 0.9 || cmp > 1.1)
                {
                    found = false;
                    break;
                }
            }

            if (found)
            {
                for (unsigned j = 0; j < 5; ++j)
                {
                    Matrix2D<int>::Peak& peak = pointPeaks[i + j].peak;
                    noteLines.emplace_back(
                            M_PI * peak.point.y / (float) LINE_ACCUMULATION_ANGLES,
                            peak.point.x - rmax,
                            pointDiff[i + j]);
                }
            }

            //LOGD(TAG, "%d %d", pointPeaks[i].yMid, (i < pointDiff.size()) ? pointDiff[i] : -1);
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    LOGD(TAG, "Line Detection: %lld", duration.count());
}


bool Detection::scan(cv::Mat& img, std::vector<LineData>& noteLines, std::vector<Matrix2D<int>>& scans)
{
    auto start = std::chrono::high_resolution_clock::now();

    bool ret;

    if (noteLines.size() >= 5)
    {
        LOGD(TAG, "spacing:%f", noteLines[0].spacing);

        int threshold = 200;
        float scales[5] = {
                noteLines[0].spacing / 16.5f, noteLines[0].spacing / 10.0f,
                noteLines[0].spacing / 10.0f,noteLines[0].spacing / 15.0f,
                noteLines[0].spacing / 10.0f
        };

        Matrix2D<uint8_t> matFromImg(img);
        Matrix2D<Float2> grad;
        matFromImg.gradient(grad);

        for (unsigned i = 0; i < rTables.size(); ++i)
        {
            if (scans.size() <= i)
                scans.emplace_back(grad.height(), grad.width(), 0);
            else
            {
                scans[i].resize(grad.height(), grad.width());
                scans[i].fill(0);
            }
        }

        unsigned long countg = 0;
        unsigned long countb = 0;

        for (unsigned y = 0; y < matFromImg.height(); ++y)
        {
            for (unsigned x = 0; x < matFromImg.width(); ++x)
            {
                Float2& data = grad.at(y, x);
                if (data.magnitude() < threshold)
                    continue;

                unsigned angle = (unsigned) ((OBJ_RTABLE_ANGLES - 1) * (0.5f + (data.atan2() / (2 * M_PI))));

                for (unsigned i = 0; i < rTables.size(); ++i)
                {
                    Float2& rtd = rTables[i][angle];
                    int yy = y - rtd.y * scales[i];
                    if (yy < 0 || yy >= grad.height())
                    {
                        countb++;
                        continue;
                    }

                    int xx = x - rtd.x * scales[i];
                    if (xx < 0 || xx >= grad.width())
                    {
                        countb++;
                        continue;
                    }

                    scans[i].at(yy, xx)++;
                    countg++;
                }
            }
        }
        LOGD(TAG, "%lu %lu", countg, countb);
        ret = true;
    }
    else
        ret = false;

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    LOGD(TAG, "Scan: %lld", duration.count());

    return ret;
}
