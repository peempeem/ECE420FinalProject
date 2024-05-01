#pragma once

#include <chrono>

class Rate
{
public:
    Rate(float hertz=0);

    void setMs(unsigned ms, bool keepStage=false);
    void setHertz(float hertz, bool keepStage=false);

    void enable();
    void disable();

    void reset();
    bool isReady(bool roll=true);

    float getStage(bool roll=false);
    float getStageRamp(bool roll=false);
    float getStageSin(float offset=0, bool roll=false);
    float getStageCos(float offset=0, bool roll=false);

private:
    unsigned long _inverseRate;
    unsigned long _start;
    unsigned long _last;
    bool _enabled;
};
