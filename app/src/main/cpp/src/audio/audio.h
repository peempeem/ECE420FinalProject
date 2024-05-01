#pragma once

#include <vector>
#include "note.h"

namespace AudioAnalyzer
{
    bool init();
    void pause();
    void resume();
    void deinit();

    const MusicNote::Data* getCurrentNote();
    void playNote(const MusicNote::Data* note);
    void getFFT(std::vector<float>& fft);
    void getAutoCorrelation(std::vector<float>& autoCorr);
};