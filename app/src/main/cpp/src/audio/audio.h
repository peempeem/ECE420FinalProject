#pragma once

#include <vector>

namespace AudioAnalyzer
{
    bool init();
    void pause();
    void resume();
    void deinit();

    void getFFT(std::vector<float>& fft);
    void getAutoCorrelation(std::vector<float>& autoCorr);
};