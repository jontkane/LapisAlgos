#include"LidarStreamerEmpty.hpp"

namespace lapis {
    LidarStreamerEmpty::LidarStreamerEmpty(const CoordRef& crs, const Extent& extent) : _crs(crs), _extent(extent) {}
    const std::span<const LasPoint> LidarStreamerEmpty::getPoints(size_t n) {
        return {};
    }
    bool LidarStreamerEmpty::hasMorePoints() const {
        return false;
    }
    size_t LidarStreamerEmpty::nPoints() const {
        return 0;
    }
    size_t LidarStreamerEmpty::nPointsRemaining() const {
        return 0;
    }
    const CoordRef& LidarStreamerEmpty::getCoordRef() const {
        return _crs;
    }
    const Extent& LidarStreamerEmpty::getExtent() const {
        return _extent;
    }
    void LidarStreamerEmpty::reset() {}
    bool LidarStreamerEmpty::_canModifyInPlace() const
    {
        return true;
    }
}