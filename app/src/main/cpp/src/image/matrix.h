#pragma once

#include <vector>

template<class T>
class Matrix2D
{
    public:
        class Itterator
        {
            public:
                Itterator(unsigned idx, T* data);

                T& operator*();
                T* operator->();
                Itterator& operator++();
                Itterator operator++(int);
                bool operator!=(const Itterator& other) const;
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

        unsigned width();
        unsigned height();

        T& at(Point pt);
        T& at(unsigned y, unsigned x);

        void resize(unsigned y, unsigned x);
        void fill(const T& data);

        std::vector<Peak> findPeaks(T min, unsigned ky=20, unsigned kx=6);

        Itterator begin();
        Itterator end();

    private:
        unsigned _y;
        unsigned _x;
        unsigned _size;
        std::unique_ptr<T[]> _data;
        std::unique_ptr<bool> _visit;

        void _create();
};

#include "matrix.hpp"