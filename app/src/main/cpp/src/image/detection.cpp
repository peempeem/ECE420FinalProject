#include "detection.h"
#include "math.h"
#include "../util/log.h"
#include <map>

#define LINE_ACCUMULATION_ANGLES 200

void getLines(cv::Mat& img, std::vector<LineData>& noteLines, std::vector<LineData>& allLines) {
    static Matrix2D<int> accumulation;
    static float cosValues[LINE_ACCUMULATION_ANGLES];
    static float sinValues[LINE_ACCUMULATION_ANGLES];
    static bool allocated = false;

    if (!allocated) {
        for (unsigned i = 0; i < LINE_ACCUMULATION_ANGLES; ++i) {
            cosValues[i] = cosf(M_PI * i / (float) LINE_ACCUMULATION_ANGLES);
            sinValues[i] = sinf(M_PI * i / (float) LINE_ACCUMULATION_ANGLES);
        }
        allocated = true;
    }

    auto start = std::chrono::high_resolution_clock::now();

    // diagonal of image
    int rmax = sqrtf(img.rows * img.rows + img.cols * img.cols);
    accumulation.resize(LINE_ACCUMULATION_ANGLES, 2 * rmax);
    accumulation.fill(0);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    LOGD(TAG, "Zero Fill: %lld", duration.count());

    start = std::chrono::high_resolution_clock::now();

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

    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    LOGD(TAG, "Accumulation: %lld", duration.count());

    start = std::chrono::high_resolution_clock::now();

    auto rawPeaks = accumulation.findPeaks(250, 16, 8);

    for (auto& peak : rawPeaks)
        allLines.emplace_back(M_PI * peak.point.y / (float) LINE_ACCUMULATION_ANGLES,(int) peak.point.x - rmax);

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

    if (pointPeaks.size() < 5)
        return;

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
                noteLines.emplace_back(M_PI * peak.point.y / (float) LINE_ACCUMULATION_ANGLES,
                                  (int) peak.point.x - rmax);
            }
        }

        LOGD(TAG, "%d %d", pointPeaks[i].yMid, (i < pointDiff.size()) ? pointDiff[i] : -1);
    }




//    for (auto& peak : rawPeaks)
//        data.emplace_back(((float) M_PI) * peak.point.y / (float) LINE_ACCUMULATION_ANGLES, (int) peak.point.x - rmax);



    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    LOGD(TAG, "Peaks: %lld", duration.count());


//    unsigned dBlockX = 10;
//    unsigned dBlockY = 4;
//    unsigned maxBlockX = ceilf(2 * rmax / (float) dBlockX);
//    unsigned maxBlockY = ceilf(314 / (float) dBlockY);
//
//    for (unsigned blockY = 0; blockY < maxBlockY; ++blockY)
//    {
//        unsigned yMin = blockY * dBlockY;
//        unsigned kyMax = yMax + dBlockY;
//        if (yMax >= 314)
//            yMax = 313;
//
//        for (unsigned blockX = 0; blockX < maxBlockX; ++blockX)
//        {
//            unsigned xMin =
//            unsigned xMax = (blockX + 1) * dBlockX;
//            if (xMax >= 2 * rmax)
//                xMax = 2 * rmax - 1;
//
//            for (unsigned y = blockY * db)
//        }
//    }

}

void get_gradient(cv::Mat& pic, std::vector<std::vector<std::vector<float>>>& grad)
{
    int16_t x_kernel[3][3] = {{-1,0,1},{-1,0,1},{-1,0,1}};
    int16_t y_kernel[3][3] = {{-1,-1,-1},{0,0,0},{1,1,1}};


    for (int i = 0; i<pic.rows; i++){
        for( int j=0; j<pic.cols; j++){

            int startx = i-1;
            int endx = i+1;
            int starty = j-1;
            int endy = j+1;

            for(int k = starty; k<=endy;k++) {
                for (int l = startx; l <= endx; l++) {
                    if(k>=0 && l>=0 && k<pic.cols && l<pic.rows){
                        grad[i][j][0]+=x_kernel[l-i+1][k-j+1]*pic.at<int>(l,k);
                        grad[i][j][1]+=y_kernel[l-i+1][k-j+1]*pic.at<int>(l,k);
                    }
                }
            }
        }
    }
}

void train(cv::Mat& pic, int threshold, std::vector<std::vector<float>>& rtable)
{
    //static cv::Mat pic;
    //cv::cvtColor(img, pic, cv::COLOR_BGRA2GRAY);

    std::vector<std::vector<std::vector<float>>> traingrad(pic.rows,std::vector<std::vector<float>>(pic.cols,std::vector<float>(2)));

    get_gradient(pic, traingrad);

    int center[2]={(int) pic.rows/2, (int) pic.cols/2};

    float gradient_mag;
    int direction;

    for(int i = 1; i < pic.rows-1; i++){
        for(int j = 1; j<pic.cols-1; j++){
            gradient_mag=pow(pow(traingrad[i][j][0],2)+pow(traingrad[i][j][1],2),0.5);
            if (gradient_mag>threshold){
                direction = (int) atan2f(traingrad[i][j][0],traingrad[i][j][1])/M_PI*180;
                if(direction<0)
                    direction+=360;
                rtable[direction][0]=i-center[0];
                rtable[direction][1]=j-center[1];

            }
        }
    }
}


void scan(cv::Mat& img,
          std::vector<std::vector<std::vector<float>>>& scan_out,
          std::vector<std::vector<float>>& rtablenote,
          std::vector<std::vector<float>>& rtabletreble,
          std::vector<std::vector<float>>& rtablebass,
          std::vector<std::vector<float>>& rtablesharp,
          std::vector<std::vector<float>>& rtableflat,
          float spacing)
{
    int threshold = 400;
    int xcoord;
    int ycoord;
    int direction;
    float gradient_mag;

    float scales[5] = {spacing/16.5f, spacing/10, spacing/10,spacing/15, spacing/10};
    std::vector<std::vector<std::vector<float>>> rtables = {rtablenote, rtabletreble, rtablebass, rtablesharp, rtableflat};


    std::vector<std::vector<std::vector<float>>> gradient_pic (img.rows,std::vector<std::vector<float>>(img.cols,std::vector<float>(2)));
    get_gradient(img, gradient_pic);

    for(int i = 1; i<img.rows-1; i++){
        for(int j = 1; j<img.cols-1; j++){
            gradient_mag=pow(pow(gradient_pic[i][j][0],2)+pow(gradient_pic[i][j][1],2),0.5);
            if (gradient_mag>threshold){
                direction = (int) atan2f(gradient_pic[i][j][0],gradient_pic[i][j][1])/M_PI*180;
                if (direction<0)
                    direction+=360;
                for(int k = 0; k<5; k++){
                    xcoord = (int) (i - rtables[k][direction][0]*scales[k]);
                    ycoord = (int) (j - rtables[k][direction][1]*scales[k]);
                    for(int xvar = xcoord-1; xvar<xcoord+2; xvar++){
                        for(int yvar = ycoord-1; yvar<ycoord+2; yvar++){
                            if(xvar>=0 && yvar>=0 && xvar<img.rows && yvar<img.cols){
                                scan_out[xvar][yvar][k]+=100;
                            }
                        }
                    }
                }
            }
        }
    }
}

int ddpeaks(std::vector<std::vector<float>>& array,
             std::vector<std::vector<int>>& peaks,
             int threshold, float spacing, int cols, int rows)
{
    int rightbound;
    int leftbound;
    int topbound;
    int bottombound;
    int areamax;
    int peakidx = 0;

    for(int j = 1; j<cols-1; j++){
        for(int i = 1; i<rows-1; i++){
            if(array[i][j]>threshold){
                topbound = (int)(i-spacing*1.3f);
                bottombound = (int)(i+spacing*1.3f);
                leftbound=(int)(j-spacing*1.3f);
                rightbound=(int)(j+spacing*1.3f);

                if(leftbound<3)
                    leftbound=3;
                if(rightbound>cols-3)
                    rightbound=cols-3;
                if(topbound<3)
                    topbound=3;
                if(bottombound>rows-3)
                    bottombound=rows-3;

                areamax=0;

                for(int l = leftbound; l<rightbound; l++){
                    for(int m = topbound; m<bottombound; m++){
                        if(array[m][l]>areamax)
                            areamax=array[m][l];
                    }
                }

                if (array[i][j]==areamax){
                    peaks[peakidx][0]=i;
                    peaks[peakidx][1]=j;
                    peakidx+=1;
                    array[i][j]+=1;
                }

            }
        }
    }
    return peakidx;
}