#include "matrix.h"
#include "../util/log.h"
#include <map>
#include <queue>

template<class T>
Matrix2D<T>::Itterator::Itterator(unsigned int idx, T* data) : _idx(idx), _data(data)
{

}

template<class T>
T& Matrix2D<T>::Itterator::operator*()
{
    return _data[_idx];
}

template<class T>
T* Matrix2D<T>::Itterator::operator->()
{
    return &_data[_idx];
}

template<class T> typename
Matrix2D<T>::Itterator& Matrix2D<T>::Itterator::operator++()
{
    _idx++;
    return *this;
}

template<class T> typename
Matrix2D<T>::Itterator Matrix2D<T>::Itterator::operator++(int)
{
    return Itterator(_idx + 1, _data);
}

template<class T>
bool Matrix2D<T>::Itterator::operator!=(const Itterator& other) const
{
    return _idx = other._idx;
}

template<class T>
unsigned Matrix2D<T>::Itterator::idx() const
{
    return _idx;
}

template<class T>
Matrix2D<T>::Matrix2D() : _y(0), _x(0)
{

}

template<class T>
Matrix2D<T>::Matrix2D(unsigned y, unsigned x) : _y(y), _x(x)
{
    _create();
}

template<class T>
Matrix2D<T>::Matrix2D(unsigned y, unsigned x, const T& data) : _y(y), _x(x)
{
    _create();
    fill(data);
}

template<class T>
unsigned Matrix2D<T>::width()
{
    return _x;
}

template<class T>
unsigned Matrix2D<T>::height()
{
    return _y;
}

template<class T>
T& Matrix2D<T>::at(Point pt)
{
    return _data[pt.y * _x + pt.x];
}

template<class T>
T& Matrix2D<T>::at(unsigned y, unsigned x)
{
    return _data[y * _x + x];
}

template<class T>
void Matrix2D<T>::resize(unsigned y, unsigned x)
{
    if (y != _y || x != _x)
    {
        _y = y;
        _x = x;
        _create();
    }
}

template<class T>
void Matrix2D<T>::fill(const T& data)
{
    for (unsigned i = 0; i < _size; ++i)
        _data[i] = data;
}

template<class T>
std::vector<typename Matrix2D<T>::Peak> Matrix2D<T>::findPeaks(T min, unsigned ky, unsigned kx)
{
    std::multimap<T, Point, std::greater<unsigned>> map;

    for (unsigned y = 0; y < height(); ++y)
    {
        for (unsigned x = 0; x < width(); ++x)
        {
            T &curr = at(y, x);
            if (curr < min ||
                (x > 1 && curr < at(y, x - 1)) ||
                (x < width() - 1 && curr < at(y, x + 1)) ||
                (y > 1 && curr < at(y - 1, x)) ||
                (y < height() - 1 && curr < at(y + 1, x)))
                continue;
            map.insert({curr, {y, x}});
        }
    }

    struct ToVisit
    {
        Point point;
        unsigned yHops;
        unsigned xHops;

        ToVisit() {}

        ToVisit(Point point, unsigned yHops, unsigned xHops) : point(point), yHops(yHops),
                                                               xHops(xHops) {}
    };

    std::vector<bool> visited(_size, false);
    std::queue<ToVisit> toVisit;
    std::vector<Peak> peaks;


    for (auto it = map.begin(); it != map.end(); ++it)
    {
        if (visited[it->second.y * _x + it->second.x])
            continue;

        toVisit.emplace(it->second, ky, kx);

        peaks.emplace_back(at(it->second), it->second);

        while (!toVisit.empty())
        {
            ToVisit &tv = toVisit.front();
            unsigned idx = tv.point.y * _x + tv.point.x;

            if (!visited[idx])
            {
                if (tv.xHops)
                {
                    if (tv.point.x > 0)
                        toVisit.emplace(Point(tv.point.y, tv.point.x - 1), tv.yHops, tv.xHops - 1);
                    if (tv.point.x < _x - 1)
                        toVisit.emplace(Point(tv.point.y, tv.point.x + 1), tv.yHops, tv.xHops - 1);
                }
                if (tv.yHops)
                {
                    if (tv.point.y > 0)
                        toVisit.emplace(Point(tv.point.y - 1, tv.point.x), tv.yHops - 1, tv.xHops);
                    if (tv.point.y < _y - 1)
                        toVisit.emplace(Point(tv.point.y + 1, tv.point.x), tv.yHops - 1, tv.xHops);

                }

                visited[idx] = true;
            }

            toVisit.pop();
        }
    }

    return peaks;
}

template<class T> typename
Matrix2D<T>::Itterator Matrix2D<T>::begin()
{
    return Itterator(0, _data);
}

template<class T> typename
Matrix2D<T>::Itterator Matrix2D<T>::end()
{
    return Itterator(_size, _data);
}

template<class T>
void Matrix2D<T>::_create()
{
    _size = _y * _x;
    _data = std::unique_ptr<T[]>(new T[_size]);
}
