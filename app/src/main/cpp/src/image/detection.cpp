#include "detection.h"

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

    double gradient_mag;
    double direction;

    for(int i = 1; i < pic.rows-1; i++){
        for(int j = 1; j<pic.cols-1; j++){
            gradient_mag=pow(pow(traingrad[i][j][0],2)+pow(traingrad[i][j][1],2),0.5);
            if (gradient_mag>threshold){
                if(traingrad[i][j][0]>=0 && traingrad[i][j][1]>0)
                    direction=atan(traingrad[i][j][0]/traingrad[i][j][1])/PI*180;
                else if(traingrad[i][j][0]>=0 && traingrad[i][j][1]<0)
                    direction=atan(-traingrad[i][j][0]/traingrad[i][j][1])/PI*180+90;
                else if(traingrad[i][j][0]<0 && traingrad[i][j][1]<0)
                    direction=atan(traingrad[i][j][0]/traingrad[i][j][1])/PI*180+180;
                else if(traingrad[i][j][0]<0 && traingrad[i][j][1]>0)
                    direction=atan(-traingrad[i][j][0]/traingrad[i][j][1])/PI*180+270;
                else if(traingrad[i][j][1]==0 && traingrad[i][j][0]>0)
                    direction = 90;
                else
                    direction=270;

                rtable[direction][0]=i-center[0];
                rtable[direction][1]=j-center[1];

            }
        }
    }
}