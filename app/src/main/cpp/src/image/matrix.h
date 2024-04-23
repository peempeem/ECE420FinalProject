#pragma once

#include <vector>
#include <opencv2/core.hpp>
#include <math.h>

struct Float2
{
    float y;
    float x;

    Float2() {}
    Float2(float y, float x) : y(y), x(x) {}

    float magnitude() { return sqrtf(x * x + y * y); }
    float atan2() { return atan2f(y, x); }
};

template<class T>
class Matrix2D
{
    public:
        class Iterator
        {
            public:
                Iterator(unsigned idx, T* data);

                T& operator*();
                T* operator->();
                Iterator& operator++();
                Iterator operator++(int);
                bool operator!=(const Iterator& other) const;
                unsigned idx() const;

            private:
                unsigned _idx;
                T* _data;
        };

        struct Point
        {
            unsigned y;
            unsigned x;

            Point() {}
            Point(unsigned y, unsigned x) : y(y), x(x) {}
        };

        struct Peak
        {
            T energy;
            Point point;

            Peak() {}
            Peak(T energy, Point point) : energy(energy), point(point) {}
        };

        Matrix2D();
        Matrix2D(unsigned y, unsigned x);
        Matrix2D(unsigned y, unsigned x, const T& data);
        Matrix2D(unsigned y, unsigned x, const T* data);
        Matrix2D(cv::Mat& mat);

        unsigned width();
        unsigned height();

        T& at(Point pt);
        T& at(unsigned y, unsigned x);

        void resize(unsigned y, unsigned x);
        void fill(const T& data);

        std::vector<Peak> peaks(T min, unsigned ky=20, unsigned kx=6);
        void gradient(Matrix2D<Float2>& grad);

        Iterator begin();
        Iterator end();

    private:
        unsigned _y;
        unsigned _x;
        unsigned _size;
        std::unique_ptr<T[]> _data;
        std::unique_ptr<bool> _visit;

        void _create();
};

#include "matrix.hpp"