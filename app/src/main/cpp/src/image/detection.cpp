#include "detection.h"
#include "math.h"
#include "../util/log.h"
#include "img_storage.h"
#include <map>
#include <list>

#define LINE_ACCUMULATION_ANGLES 360
#define OBJ_RTABLE_ANGLES 10
#define OBJ_RTABLE_MAX_OVERLAP 100
#define RTABLE_THRESHOLD 100

#define DIV_ROUND_CLOSEST(n, d) ((((n) < 0) == ((d) < 0)) ? (((n) + (d)/2)/(d)) : (((n) - (d)/2)/(d)))

static float cosValues[LINE_ACCUMULATION_ANGLES];
static float sinValues[LINE_ACCUMULATION_ANGLES];
static std::vector<unsigned> objPointCounts;

std::vector<std::vector<std::vector<Float2>>> rTables;

void train(Matrix2D<int>& encoding, int threshold)
{
    Matrix2D<Float2> grad;
    encoding.gradient(grad);

    rTables.emplace_back(OBJ_RTABLE_ANGLES, std::vector<Float2>());
    std::vector<std::vector<Float2>>& rTable = rTables.back();
    std::vector<std::map<float, Float2, std::greater<float>>> tempRT(OBJ_RTABLE_ANGLES);

    unsigned topY = encoding.height();
    for (unsigned y = 0; y < encoding.height(); ++y)
    {
        for (unsigned x = 0; x < encoding.width(); ++x)
        {
            if (encoding.at(y, x) != std::numeric_limits<uint8_t>::max())
            {
                topY = y;
                break;
            }
        }
        if (topY != encoding.height())
            break;
    }

    unsigned leftX = encoding.width();
    for (unsigned x = 0; x < encoding.width(); ++x)
    {
        for (unsigned y = 0; y < encoding.height(); ++y)
        {
            if (encoding.at(y, x) != std::numeric_limits<uint8_t>::max())
            {
                leftX = x;
                break;
            }
        }
        if (leftX != encoding.width())
            break;
    }

    unsigned rightX = 0;
    for (int x = encoding.width() - 1; x > 0; --x)
    {
        for (unsigned y = 0; y < encoding.height(); ++y)
        {
            if (encoding.at(y, x) != std::numeric_limits<uint8_t>::max())
            {
                rightX = x;
                break;
            }
        }
        if (rightX != 0)
            break;
    }

    unsigned botY = 0;
    for (int y = encoding.height() - 1; y >= 0; --y)
    {
        for (unsigned x = 0; x < encoding.width(); ++x)
        {
            if (encoding.at(y, x) != std::numeric_limits<uint8_t>::max())
            {
                botY = y;
                break;
            }
        }
        if (botY != 0)
            break;
    }

    float centerX = (leftX + rightX) / 2.0f;
    float centerY = (topY + botY) / 2.0f;

    float height = botY - topY;

    for (unsigned y = 0; y < encoding.height(); ++y)
    {
        for (unsigned x = 0; x < encoding.width(); ++x)
        {
            Float2& data = grad.at(y, x);
            if (data.magnitude() >= threshold)
            {
                unsigned angle = (unsigned) ((OBJ_RTABLE_ANGLES - 1) * (0.5f + (data.atan2() / (2 * M_PI))));
                tempRT[angle].insert({data.magnitude(), Float2((y - centerY) / height, (x - centerX) / height)});
            }
        }
    }

    unsigned count = 0;
    for (unsigned i = 0; i < OBJ_RTABLE_ANGLES; ++i)
    {
        unsigned pulled = 0;
        for (auto& item : tempRT[i])
        {
            if (pulled >= OBJ_RTABLE_MAX_OVERLAP)
                break;
            rTable[i].emplace_back(item.second);
            pulled++;
            count++;
        }
    }
    objPointCounts.push_back(count);
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

bool intersection(Detection::Linef& l1, Detection::Linef& l2, Detection::Pointf& p)
{
    if (fabs(l1.m - l2.m) < (float) 1e-6)
        return false;
    p.x = (l2.b - l1.b) / (l1.m - l2.m);
    p.y = l1.m * p.x + l1.b;
    return true;
}

float lineToPointDistance(Detection::Linef& line, Detection::Pointf& point)
{
    return fabs((-line.m * point.x) + point.y - line.b) / sqrtf(line.m * line.m + line.b * line.b);
}

float lineAngleToPointDistance(Detection::Linef& line, Detection::Pointf& point)
{
    return fabs(cosf(line.m) * (line.b - point.y) - sinf(line.m) * -point.x);
}

void Detection::getMusicLines(cv::Mat& img, std::vector<Music>& musicLines, std::vector<LineData>& allLines)
{
    static Matrix2D<int> accumulation;

    // diagonal of image
    int rmax = sqrtf(img.rows * img.rows + img.cols * img.cols);
    accumulation.resize(LINE_ACCUMULATION_ANGLES, 2 * rmax);
    accumulation.fill(0);

    // accumulation
    for (unsigned r = 0; r < img.rows; ++r)
    {
        for (unsigned c = 0; c < img.cols; ++c)
        {
            if (img.at<uint8_t>(r, c) < 127)
                continue;

            for (unsigned theta = 0; theta < LINE_ACCUMULATION_ANGLES; ++theta)
            {
                int dist = roundf(c * cosValues[theta] + r * sinValues[theta]);
                if (dist >= -rmax && dist < rmax)
                    accumulation.at(theta, dist + rmax)++;
            }
        }
    }

    auto rawPeaks = accumulation.peaks(300, 10, 5);

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

    std::sort(pointPeaks.begin(), pointPeaks.end());

    std::list<std::list<PointPeak>> lineBursts;
    lineBursts.emplace_back();
    for (auto& pp : pointPeaks)
    {
        if (lineBursts.back().empty() || pp.yMid - lineBursts.back().back().yMid < 0.15f * accumulation.height())
            lineBursts.back().emplace_back(pp);
        else
        {
            lineBursts.emplace_back();
            lineBursts.back().emplace_back(pp);
        }
    }

    lineBursts.remove_if([](const auto& burst) { return burst.size() < 5; });

    for (auto& burst : lineBursts)
    {
        while (burst.size() > 5)
        {
            std::vector<std::list<PointPeak>::iterator> iterators;
            float angleAvg = 0;
            for (auto it = burst.begin(); it != burst.end(); ++it)
            {
                iterators.push_back(it);
                angleAvg += it->peak.point.y;
            }

            std::vector<float> newAvg(burst.size());

            for (unsigned i = 0; i < burst.size(); ++i)
                newAvg[i] = fabs(iterators[i]->peak.point.y - (angleAvg - iterators[i]->peak.point.y) / (burst.size() - 1));

            burst.erase(iterators[std::max_element(newAvg.begin(), newAvg.end()) - newAvg.begin()]);
        }

        unsigned min = std::numeric_limits<unsigned>::max();
        unsigned max = std::numeric_limits<unsigned>::min();
        float avgAngle = 0;
        float distAvg = 0;
        for (auto& pp : burst)
        {
            if (pp.peak.point.y < min)
                min = pp.peak.point.y;
            else if (pp.peak.point.y > max)
                max = pp.peak.point.y;
            avgAngle += pp.peak.point.y;
            distAvg += pp.peak.point.x;
        }

        if (M_PI * (max - min) / (float) LINE_ACCUMULATION_ANGLES > 5)
            continue;

        int diff[4];
        auto it = burst.begin();
        for (unsigned i = 0; i < 4; ++i)
        {
            int x1 = it->peak.point.x;
            ++it;
            int x2 = it->peak.point.x;
            diff[i] = x2 - x1;
        }

        bool invalid = false;
        for (unsigned i = 1; i < 4; ++i)
        {
            float dd = fabs(diff[i] / (float) diff[0]);
            if (dd > 1.4f || dd < 0.6f)
            {
                invalid = true;
                break;
            }
        }
        if (invalid)
            continue;

        avgAngle = M_PI * (avgAngle / burst.size()) / (float) LINE_ACCUMULATION_ANGLES;
        distAvg = (distAvg / burst.size()) - rmax;

        musicLines.emplace_back();

        musicLines.back().spacing = (burst.back().peak.point.x - burst.front().peak.point.x) / burst.size();
        musicLines.back().angle = avgAngle;
        musicLines.back().middle = distAvg;

        unsigned i = 0;
        for (auto& pp : burst)
        {
            musicLines.back().distance[i] = pp.peak.point.x - rmax;
            ++i;
        }
    }
}

cv::String Detection::getNote(std::vector<LineData>& noteLines, int position)
{
    float a;
    float b;
    int y0;
    int y1;
    int y2;
    int distance;
    int prev_distance=1000;
    int noteidx;
    int spacing=noteLines[0].spacing;

    std::vector<int>centers=std::vector<int>(noteLines.size()/5);
    std::vector<cv::String>noteMap={"G","F","E","D","C","B","A"};

    for(int i = 0; i<centers.size(); i++){
        a = cosf(noteLines[5*i+2].theta);
        b = sinf(noteLines[5*i+2].theta);

        y0 = b * noteLines[5*i+2].distance;

        y1 = y0 + (int) 10000 * a;
        y2 = y0 - (int) 10000 * a;
        centers[i]=(int)round((y1+y2)/2);
    }

    for(int i = 0; i<centers.size(); i++){
        distance = position-centers[i];
        if (abs(distance)>abs(prev_distance)){
            distance=prev_distance;
        }
        else{
            prev_distance=distance;
        }
    }

    noteidx=(DIV_ROUND_CLOSEST(distance, DIV_ROUND_CLOSEST(spacing,2))+3)%7;

    while(noteidx<0)
        noteidx+=7;

    return noteMap[noteidx];
}

bool Detection::scan(cv::Mat& img, std::vector<Music>& musicLines)
{
    static std::vector<Matrix2D<int>> scans;

    if (musicLines.empty())
        return false;

    float scales[5] = {
            musicLines.front().spacing,
            musicLines.front().spacing * 7.2295f,
            musicLines.front().spacing * 3.5245f,
            musicLines.front().spacing / 15.0f,         // TODO
            musicLines.front().spacing / 10.0f          // TODO
    };
    std::vector<unsigned> thresholds = {
            (unsigned) (objPointCounts[0] * sqrtf(musicLines.front().spacing) / 10),
            (unsigned) (objPointCounts[1] * sqrtf(musicLines.front().spacing) / 25),
            (unsigned) (objPointCounts[2] * sqrtf(musicLines.front().spacing) / 15),
            (unsigned) (objPointCounts[3] * sqrtf(musicLines.front().spacing) / 15),
            (unsigned) (objPointCounts[4] * sqrtf(musicLines.front().spacing) / 15)
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

    for (unsigned y = 0; y < grad.height(); ++y)
    {
        for (unsigned x = 0; x < grad.width(); ++x)
        {
            Float2& data = grad.at(y, x);
            if (fabs(data.magnitude()) < RTABLE_THRESHOLD)
                continue;

            unsigned angle = (unsigned) ((OBJ_RTABLE_ANGLES - 1) * (0.5f + (data.atan2() / (2 * M_PI))));

            for (unsigned i = 0; i < rTables.size(); ++i)
            {
                std::vector<Float2>& rTableData = rTables[i][angle];
                for (auto& f2 : rTableData)
                {
                    int yy = y - f2.y * scales[i];
                    if (yy < 0 || yy >= grad.height())
                        continue;

                    int xx = x - f2.x * scales[i];
                    if (xx < 0 || xx >= grad.width())
                        continue;

                    scans[i].at(yy, xx)++;
                }
            }
        }
    }

    std::vector<Linef> lines(musicLines.size());
    std::vector<float> distance(musicLines.size());

    LOGD(TAG, "log lines:");
    for (unsigned i = 0; i < musicLines.size(); ++i)
    {
        lines[i].m = musicLines[i].angle - M_PI / 2.0f;
        float x = cosf(musicLines[i].angle) * musicLines[i].middle;
        float y = sinf(musicLines[i].angle) * musicLines[i].middle;
        lines[i].b = y - x * tanf(musicLines[i].angle - M_PI / 2.0f);
        LOGD(TAG, "%f, %f", lines[i].m, lines[i].b);
    }
    LOGD(TAG, "done");

    auto treblePeaks = scans[1].peaks(thresholds[1], 20, 20);
    auto bassPeaks = scans[2].peaks(thresholds[2], 20, 20);

    LOGD(TAG, "treble peaks: %u", (unsigned) treblePeaks.size());
    LOGD(TAG, "bass peaks: %u", (unsigned) bassPeaks.size());

    struct ClefData
    {
        bool tpSet = false;
        bool bpSet = false;
        Matrix2D<int>::Peak tp;
        Matrix2D<int>::Peak bp;
        float tdist;
        float bdist;
    };

    std::vector<ClefData> clefFinder(musicLines.size());

    for (auto& peak : treblePeaks)
    {
        for (unsigned j = 0; j < distance.size(); ++j)
        {
            Pointf p(peak.point.x, peak.point.y);
            distance[j] = lineAngleToPointDistance(lines[j], p);
        }

        auto it = std::min_element(std::begin(distance), std::end(distance));

        if (*it > musicLines.front().spacing * 2)
            continue;

        unsigned idx = it - distance.begin();
        if (!clefFinder[idx].tpSet)
        {
            clefFinder[idx].tp = peak;
            clefFinder[idx].tpSet = true;
            clefFinder[idx].tdist = *it;
        }
    }

    for (auto& peak : bassPeaks)
    {
        for (unsigned j = 0; j < distance.size(); ++j)
        {
            Pointf p(peak.point.x, peak.point.y);
            distance[j] = lineAngleToPointDistance(lines[j], p);
        }

        auto it = std::min_element(std::begin(distance), std::end(distance));

        if (*it > musicLines.front().spacing * 2)
            continue;

        unsigned idx = it - distance.begin();
        if (!clefFinder[idx].bpSet)
        {
            clefFinder[idx].bp = peak;
            clefFinder[idx].bpSet = true;
            clefFinder[idx].bdist = *it;
        }
    }

    for (unsigned i = 0; i < clefFinder.size(); ++i)
    {
        if (clefFinder[i].tpSet && !clefFinder[i].bpSet)
        {
            musicLines[i].clef = Music::Treble;
            musicLines[i].clefPos.x = clefFinder[i].tp.point.x;
            musicLines[i].clefPos.y = clefFinder[i].tp.point.y;
        }
        else if (!clefFinder[i].tpSet && clefFinder[i].bpSet)
        {
            musicLines[i].clef = Music::Bass;
            musicLines[i].clefPos.x = clefFinder[i].bp.point.x;
            musicLines[i].clefPos.y = clefFinder[i].bp.point.y;
        }
        else if (clefFinder[i].tpSet && clefFinder[i].bpSet)
        {
            if (clefFinder[i].tp.energy > clefFinder[i].bp.energy && clefFinder[i].tdist < clefFinder[i].bdist)
            {
                musicLines[i].clef = Music::Treble;
                musicLines[i].clefPos.x = clefFinder[i].tp.point.x;
                musicLines[i].clefPos.y = clefFinder[i].tp.point.y;
            }
            else
            {
                musicLines[i].clef = Music::Bass;
                musicLines[i].clefPos.x = clefFinder[i].bp.point.x;
                musicLines[i].clefPos.y = clefFinder[i].bp.point.y;
            }
        }
    }

    auto notePeaks = scans[0].peaks(thresholds[0], 16, 16);
    LOGD(TAG, "note peaks: %u", (unsigned) notePeaks.size());

    for (auto& peak : notePeaks)
    {
        for (unsigned j = 0; j < distance.size(); ++j)
        {
            Pointf p(peak.point.x, peak.point.y);
            distance[j] = lineAngleToPointDistance(lines[j], p);
        }

        auto it = std::min_element(std::begin(distance), std::end(distance));
        unsigned idx = it - distance.begin();

        if (*it > musicLines.front().spacing * 5)
            continue;

        if (musicLines[idx].clef != Music::Unknown
            && fabs((musicLines[idx].clefPos.x - peak.point.x) / musicLines.front().spacing) < 4)
            continue;

        musicLines[idx].notes.emplace_back();
        musicLines[idx].notes.back().position.x = peak.point.x;
        musicLines[idx].notes.back().position.y = peak.point.y;

        if (musicLines[idx].clef == Music::Unknown)
        {
            musicLines[idx].notes.back().data = NULL;
            continue;
        }

        Pointf p(peak.point.x, peak.point.y);
        float noteDist = lineAngleToPointDistance(lines[idx], p) / (musicLines[idx].spacing / 2.0f);

        float x = cosf(musicLines[idx].angle) * musicLines[idx].distance[2];
        float y = sinf(musicLines[idx].angle) * musicLines[idx].distance[2];
        float yy = y - (peak.point.x - x) * tanf(musicLines[idx].angle - M_PI / 2.0f);

        int nd = (peak.point.y > yy) ? -roundf(noteDist) : roundf(noteDist);

        auto scaleIT = musicNote.beginScale(MusicNote::C_MAJOR, 4);
        const MusicNote::Data* target = musicNote.fromName((musicLines[idx].clef == Music::Treble) ? "B4" : "D3");

        while (scaleIT->name[0] != target->name[0])
        {
            if (target->midi > scaleIT->midi)
                ++scaleIT;
            else if (target->midi < scaleIT->midi)
                --scaleIT;
            else
                break;
        }

        if (nd > 0)
        {
            for (unsigned i = 0; i < nd; ++i)
                ++scaleIT;
        }
        else
        {
            for (unsigned i = 0; i < abs(nd); ++i)
                --scaleIT;
        }

        musicLines[idx].notes.back().data = &*scaleIT;
    }

    return true;
}
