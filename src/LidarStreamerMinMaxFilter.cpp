#include"LidarStreamerMinMaxFilter.hpp"


namespace lapis {
    LidarStreamerMinMaxFilter::LidarStreamerMinMaxFilter(std::unique_ptr<LidarStreamer> streamer, coord_t minZ, coord_t maxZ)
        : _streamer(std::move(streamer)), _minZ(minZ), _maxZ(maxZ) {}
    const std::span<const LasPoint> LidarStreamerMinMaxFilter::getPoints(size_t n)
    {
        if (!_streamer->hasMorePoints()) {
            return {};
        }
        if (LidarStreamer::_canModifyInPlace(*_streamer)) {
            auto points = _streamer->getPoints(n);
            LasPoint* writePtr = const_cast<LasPoint*>(points.data());
            size_t writeIdx = 0;
            for (size_t i = 0; i < points.size(); ++i) {
                if (points[i].z >= _minZ && points[i].z <= _maxZ) {
                    if (i != writeIdx) {
                        writePtr[writeIdx] = points[i];
                    }
                    ++writeIdx;
                }
            }
            return std::span<const LasPoint>(points.data(), writeIdx);
        }
        else {
            _buffer.clear();
            _buffer.reserve(n);
            auto points = _streamer->getPoints(n);
            for (const auto& point : points) {
                if (point.z >= _minZ && point.z <= _maxZ) {
                    _buffer.push_back(point);
                }
            }
            return _buffer;
        }
    }
    bool LidarStreamerMinMaxFilter::hasMorePoints() const
    {
        return _streamer->hasMorePoints();
    }
    size_t LidarStreamerMinMaxFilter::nPoints() const
    {
        return _streamer->nPoints();
    }
    size_t LidarStreamerMinMaxFilter::nPointsRemaining() const
    {
        return _streamer->nPointsRemaining();
    }
    const CoordRef& LidarStreamerMinMaxFilter::getCoordRef() const
    {
        return _streamer->getCoordRef();
    }
    const Extent& LidarStreamerMinMaxFilter::getExtent() const
    {
        return _streamer->getExtent();
    }
    void LidarStreamerMinMaxFilter::reset()
    {
        _streamer->reset();
    }
    bool LidarStreamerMinMaxFilter::_canModifyInPlace() const
    {
        return true;
    }
}