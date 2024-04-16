#include "detection.h"
#include "math.h"
#include "../util/log.h"

void getLines(cv::Mat& img, std::vector<LineData>& data)
{
    static std::vector<unsigned> accumulation;
    static float cosValues[315];
    static float sinValues[315];
    static bool allocated = false;

    // diagonal of image
    int rmax = sqrtf(img.rows * img.rows + img.cols * img.cols);

    accumulation.resize(315 * rmax * 2);
    for (unsigned i = 0; i < accumulation.size(); ++i)
        accumulation[i] = 0;

    if (!allocated)
    {
        for (unsigned i = 0; i < 315; ++i)
        {
            cosValues[i] = cosf(i / 100.0f);
            sinValues[i] = sinf(i / 100.0f);
        }
        allocated = true;
    }

    // accumulation
    for (unsigned r = 0; r < img.rows; ++r)
    {
        for (unsigned c = 0; c < img.cols; ++c)
        {
            if (img.at<uint8_t>(r, c) < 127)
                continue;

            for (unsigned theta = 0; theta < 315; ++theta)
            {
                int dist = c * cosValues[theta] + r * sinValues[theta];
                if (dist > -rmax && dist < rmax)
                    accumulation[theta * 2 * rmax + dist + rmax]++;
            }
        }
    }

    for (unsigned i = 0; i < accumulation.size(); ++i)
    {
        if (accumulation[i] > img.cols / 3)
            data.emplace_back(i / (rmax * 2), (i % (2 * rmax)) - rmax);
    }
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
                        grad[i][j][0]+=x_kernel[l-i+1][k-j+1]*pic.at<double>(l,k);
                        grad[i][j][1]+=y_kernel[l-i+1][k-j+1]*pic.at<double>(l,k);
                    }
                }
            }
        }
    }
}

void train(cv::Mat& img, int threshold, std::vector<std::vector<float>>& rtable)
{
    static cv::Mat pic;
    cv::cvtColor(img, pic, cv::COLOR_BGRA2GRAY);

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
        for(int j = 1; i<img.cols-1; i++){
            gradient_mag=pow(pow(gradient_pic[i][j][0],2)+pow(gradient_pic[i][j][1],2),0.5);
            if (gradient_mag>threshold){
                direction = (int) atan2f(gradient_pic[i][j][0],gradient_pic[i][j][1])/M_PI*180;
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