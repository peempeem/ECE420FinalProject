#pragma once

#include <mutex>
#include <vector>

template<class T, unsigned LEN>
class AudioStream
{
    public:
        void putOverlapAdd(const T* buf, unsigned len);
        void get(T* buf, unsigned max);

    private:
        unsigned _idx = 0;

        std::mutex _mtx;
        std::vector<T> _data = std::vector<T>(LEN);
};

#include "stream.hpp"