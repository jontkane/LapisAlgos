#include"LidarStreamerMemory.hpp"

namespace lapis {
    LidarStreamerMemory::LidarStreamerMemory(const std::vector<LasPoint>& points, const CoordRef& crs, const Extent& e)
        : _points(points), _currentIndex(0), _crs(crs), _extent(e)
    {}
    LidarStreamerMemory::LidarStreamerMemory(std::vector<LasPoint>&& points, const CoordRef& crs, const Extent& e)
        : _points(std::move(points)), _currentIndex(0), _crs(crs), _extent(e) {}

    const std::span<const LasPoint> LidarStreamerMemory::getPoints(size_t n)
    {
        size_t toGet = std::min(n, _points.size() - _currentIndex);
        const std::span<const LasPoint> points = std::span<const LasPoint>(_points.data() + _currentIndex, toGet);
        _currentIndex += toGet;
        return points;
    }

    bool LidarStreamerMemory::hasMorePoints() const
    {
        return _currentIndex < _points.size();
    }

    size_t LidarStreamerMemory::nPoints() const
    {
        return _points.size();
    }

    size_t LidarStreamerMemory::nPointsRemaining() const
    {
        return _points.size() - _currentIndex;
    }

    const CoordRef& LidarStreamerMemory::getCoordRef() const
    {
        return _crs;
    }

    const Extent& LidarStreamerMemory::getExtent() const
    {
        return _extent;
    }

    void LidarStreamerMemory::reset()
    {
        _currentIndex = 0;
    }

    bool LidarStreamerMemory::_canModifyInPlace() const
    {
        return false;
    }

}