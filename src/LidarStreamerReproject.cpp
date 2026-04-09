#include"LidarStreamerReproject.hpp"

namespace lapis {
    LidarStreamerReproject::LidarStreamerReproject(std::unique_ptr<LidarStreamer>&& source, const CoordRef& targetCRS)
        : _source(std::move(source)) {
        _tr = &CoordTransformFactory::getTransform(_source->getCoordRef(), targetCRS);
        _e = QuadExtent(_source->getExtent(), targetCRS).outerExtent();
    }
    const std::span<const LasPoint> LidarStreamerReproject::getPoints(size_t n)
    {
        std::span<const LasPoint> sourcePoints = _source->getPoints(n);
        if (!sourcePoints.size()) {
            return sourcePoints;
        }
        if (_tr->src().isConsistent(_tr->dst())) {
            return sourcePoints;
        }
        if (LidarStreamer::_canModifyInPlace(*_source)) {
            coord_t* x = const_cast<coord_t*>(&sourcePoints[0].x);
            coord_t* y = const_cast<coord_t*>(&sourcePoints[0].y);
            coord_t* z = const_cast<coord_t*>(&sourcePoints[0].z);
            _tr->transformXYZGeneric(sourcePoints.size(), x, sizeof(LasPoint), y, sizeof(LasPoint), z, sizeof(LasPoint));
            return sourcePoints;
        }
        else {
            _buffer.assign(sourcePoints.begin(), sourcePoints.end());
            _tr->transformXYZGeneric(_buffer.size(), &_buffer[0].x, sizeof(LasPoint), &_buffer[0].y, sizeof(LasPoint), &_buffer[0].z, sizeof(LasPoint));
            return _buffer;
        }
    }
    bool LidarStreamerReproject::hasMorePoints() const
    {
        return _source->hasMorePoints();
    }
    size_t LidarStreamerReproject::nPoints() const
    {
        return _source->nPoints();
    }
    size_t LidarStreamerReproject::nPointsRemaining() const
    {
        return _source->nPointsRemaining();
    }
    const CoordRef& LidarStreamerReproject::getCoordRef() const
    {
        return _tr->dst();
    }
    const Extent& LidarStreamerReproject::getExtent() const
    {
        return _e;
    }
    void LidarStreamerReproject::reset()
    {
        _source->reset();
    }
    bool LidarStreamerReproject::_canModifyInPlace() const
    {
        return _tr->src().isConsistent(_tr->dst()) || LidarStreamer::_canModifyInPlace(*_source);
    }
}