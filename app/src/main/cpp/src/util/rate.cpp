#include "rate.h"
#include "math.h"

static unsigned long sysTime()
{
    static auto start = std::chrono::high_resolution_clock::now();
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
}

Rate::Rate(float hertz)
{
    setHertz(hertz);
}

void Rate::setMs(unsigned ms, bool keepStage)
{
    unsigned long newInverseRate;
    if (!ms)
        newInverseRate = -1;
    else
        newInverseRate = ms * 1000;

    _start = sysTime();
    if (keepStage)
    {
        unsigned long offset = newInverseRate * getStage();
        if (offset > _start)
            _start = 0;
        else
            _start -= offset;
    }

    _inverseRate = newInverseRate;
    _last = _start;
    _enabled = true;
}

void Rate::setHertz(float hertz, bool keepStage)
{
    unsigned long newInverseRate;
    if (hertz <= 0)
        newInverseRate = -1;
    else
        newInverseRate = 1e6 / hertz;

    _start = sysTime();
    if (keepStage)
    {
        unsigned long offset = newInverseRate * getStage();
        if (offset > _start)
            _start = 0;
        else
            _start -= offset;
    }

    _inverseRate = newInverseRate;
    _last = _start;
    _enabled = true;
}

void Rate::enable()
{
    _enabled = true;
}

void Rate::disable()
{
    _enabled = false;
}

void Rate::reset()
{
    _start = sysTime();
    _last = _start;
}

bool Rate::isReady(bool roll)
{
    if (!_enabled)
        return false;
    unsigned long time = sysTime();
    if (time > _inverseRate + _last)
    {
        if (roll)
            _last = time - (time - _last) % _inverseRate;
        else
            reset();
        return true;
    }
    return false;
}

float Rate::getStage(bool roll)
{
    if (roll)
        return ((sysTime() - _last) % _inverseRate) / (float) _inverseRate;
    else
        return fmin((sysTime() - _last) / (float) _inverseRate, 1);
}

float Rate::getStageRamp(bool roll)
{
    return sinf(getStage(roll) * M_PI / 2.0f);
}

float Rate::getStageSin(float offset, bool roll)
{
    return (sinf((getStage(roll) + offset) * 2 * M_PI) + 1) / 2.0f;
}

float Rate::getStageCos(float offset, bool roll)
{
    return (cosf((getStage(roll) + offset) * 2 * M_PI) + 1) / 2.0f;
}
