#include "stream.h"

template<class T, unsigned LEN>
void AudioStream<T, LEN>::putOverlapAdd(const T* buf, unsigned len)
{
    _mtx.lock();
    for (unsigned i = 0; i < len; ++i)
        _data[(_idx + i) % LEN] += buf[i];
    _mtx.unlock();
}

template<class T, unsigned LEN>
void AudioStream<T, LEN>::get(T* buf, unsigned max)
{
    _mtx.lock();
    for (unsigned i = 0; i < max; ++i)
    {
        buf[i] = _data[_idx];
        _data[_idx] = T();
        _idx = (_idx + 1) % LEN;
    }
    _mtx.unlock();
}
