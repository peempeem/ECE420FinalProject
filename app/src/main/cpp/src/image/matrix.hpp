#include "matrix.h"
#include "../util/log.h"
#include <map>
#include <queue>
#include <memory>
#include <sstream>

#define DEBUG true

template<class T>
Matrix2D<T>::Iterator::Iterator(unsigned int idx, T* data) : _idx(idx), _data(data)
{

}

template<class T>
T& Matrix2D<T>::Iterator::operator*()
{
    return _data[_idx];
}

template<class T>
T* Matrix2D<T>::Iterator::operator->()
{
    return &_data[_idx];
}

template<class T> typename
Matrix2D<T>::Iterator& Matrix2D<T>::Iterator::operator++()
{
    _idx++;
    return *this;
}

template<class T> typename
Matrix2D<T>::Iterator Matrix2D<T>::Iterator::operator++(int)
{
    return Iterator(_idx + 1, _data);
}

template<class T>
bool Matrix2D<T>::Iterator::operator!=(const Iterator& other) const
{
    return _idx = other._idx;
}

template<class T>
unsigned Matrix2D<T>::Iterator::idx() const
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
Matrix2D<T>::Matrix2D(unsigned y, unsigned x, const T* data) : _y(y), _x(x)
{
    _create();
    memcpy(_data.get(), data, _size * sizeof(T));
}

template<class T>
Matrix2D<T>::Matrix2D(cv::Mat& mat)
{
    if (!mat.isContinuous())
        throw std::invalid_argument("cv::Mat is not contiguous");
    else if (mat.elemSize() != sizeof(T))
        throw std::invalid_argument("cv::Mat data type size does not match Matrix2D<T> data type size");

    _y = mat.rows;
    _x = mat.cols;
    _create();
    memcpy(_data.get(), mat.data, _size * sizeof(T));
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
    #if DEBUG
    if (pt.y >= _y)
    {
        std::stringstream ss;
        ss << "y index " << pt.y << " is out of bounds. Max size is " << _y;
        throw std::invalid_argument(ss.str());
    }
    else if (pt.x >= _x)
    {
        std::stringstream ss;
        ss << "x index " << pt.x << " is out of bounds. Max size is " << _x;
        throw std::invalid_argument(ss.str());
    }
    #endif

    return _data[pt.y * _x + pt.x];
}

template<class T>
T& Matrix2D<T>::at(unsigned y, unsigned x)
{
    #if DEBUG
    if (y >= _y)
    {
        std::stringstream ss;
        ss << "y index " << y << " is out of bounds. Max size is " << _y;
        throw std::invalid_argument(ss.str());
    }
    else if (x >= _x)
    {
        std::stringstream ss;
        ss << "x index " << x << " is out of bounds. Max size is " << _x;
        throw std::invalid_argument(ss.str());
    }
    #endif

    return _data[y * _x + x];
}

template<class T>
void Matrix2D<T>::resize(unsigned y, unsigned x)
{
    if (y != height() || x != width())
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
std::vector<typename Matrix2D<T>::Peak> Matrix2D<T>::peaks(T min, unsigned ky, unsigned kx)
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

template<class T>
void Matrix2D<T>::gradient(Matrix2D<Float2>& grad)
{
    int xKernel[3][3] =
            {
                    {-1, 0, 1},
                    {-1, 0, 1},
                    {-1, 0, 1}
            };
    int yKernel[3][3] =
            {
                    {-1, -1, -1},
                    {0, 0, 0},
                    {1, 1, 1}
            };

    grad.resize(height(), width());
    grad.fill(Float2(0, 0));

    for (int y = 0; y < height(); ++y)
    {
        for (int x = 0; x < width(); ++x)
        {
            for (int ky = 0; ky < 3; ++ky)
            {
                int yy = y + ky - 1;
                if (yy < 0 || yy >= height())
                    continue;

                for (int kx = 0; kx < 3; ++kx)
                {
                    int xx = x + kx - 1;
                    if (xx < 0 || xx >= width())
                        continue;

                    Float2& data = grad.at(y, x);
                    data.x += xKernel[ky][kx] * at(yy, xx);
                    data.y += yKernel[ky][kx] * at(yy, xx);
                }
            }
        }
    }
}

template<class T> typename
Matrix2D<T>::Iterator Matrix2D<T>::begin()
{
    return Iterator(0, _data);
}

template<class T> typename
Matrix2D<T>::Iterator Matrix2D<T>::end()
{
    return Iterator(_size, _data);
}

template<class T>
void Matrix2D<T>::_create()
{
    _size = _y * _x;
    _data = std::unique_ptr<T[]>(new T[_size]);
}
