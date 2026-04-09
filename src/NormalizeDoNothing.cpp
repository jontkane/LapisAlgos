#include"NormalizeDoNothing.hpp"

namespace lapis {
    NormalizeDoNothing::NormalizeDoNothing(std::unique_ptr<LidarStreamer> streamer) : _source(std::move(streamer)) {}
    const std::span<const LasPoint> NormalizeDoNothing::getPoints(size_t n) {
        return _source->getPoints(n);
    }
    bool NormalizeDoNothing::hasMorePoints() const {
        return _source->hasMorePoints();
    }
    size_t NormalizeDoNothing::nPoints() const {
        return _source->nPoints();
    }
    size_t NormalizeDoNothing::nPointsRemaining() const {
        return _source->nPointsRemaining();
    }
    const CoordRef& NormalizeDoNothing::getCoordRef() const {
        return _source->getCoordRef();
    }
    const Extent& NormalizeDoNothing::getExtent() const
    {
        return _source->getExtent();
    }
    void NormalizeDoNothing::reset() {
        _source->reset();
    }
    bool NormalizeDoNothing::_canModifyInPlace() const
    {
        return LidarStreamer::_canModifyInPlace(*_source);
    }
}