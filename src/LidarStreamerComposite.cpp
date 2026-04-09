#include"LidarStreamerComposite.hpp"
#include"LidarStreamerReproject.hpp"

namespace lapis {
    LidarStreamerComposite::LidarStreamerComposite(const CoordRef& crs)
        : _crs(crs)
    {}
    void LidarStreamerComposite::addStream(std::unique_ptr<LidarStreamer> stream)
    {
        if (_crs.isEmpty()) {
            _crs = stream->getCoordRef();
        }
        if (stream->getCoordRef().isConsistent(_crs)) {
            _streams.push_back(std::move(stream));
        } else {
            std::unique_ptr<LidarStreamerReproject> reprojectedStream = std::make_unique<LidarStreamerReproject>(std::move(stream), _crs);
            _streams.push_back(std::move(reprojectedStream));
        }

        if (_streams.size() == 1) {
            _extent = _streams[0]->getExtent();
        }
        else {
            _extent = extendExtent(_extent, _streams.back()->getExtent());
        }
    }
    const std::span<const LasPoint> LidarStreamerComposite::getPoints(size_t n)
    {
        _buffer.clear();
        _buffer.reserve(n);
        if (n == 0 || _currentStreamIndex >= _streams.size()) {
            return {};
        }
        size_t pointsFetched = 0;
        while (pointsFetched < n && _currentStreamIndex < _streams.size()) {
            auto& currentStream = _streams[_currentStreamIndex];
            if (!currentStream->hasMorePoints()) {
                _currentStreamIndex++;
                continue;
            }
            size_t pointsToFetch = n - pointsFetched;
            auto points = currentStream->getPoints(pointsToFetch);
            _buffer.insert(_buffer.end(), points.begin(), points.end());
            pointsFetched += points.size();
            if (!currentStream->hasMorePoints()) {
                _currentStreamIndex++;
            }
        }
        return _buffer;
    }
    bool LidarStreamerComposite::hasMorePoints() const
    {
        if (_currentStreamIndex >= _streams.size()) {
            return false;
        }
        while (_currentStreamIndex < _streams.size()) {
            if (_streams[_currentStreamIndex]->hasMorePoints()) {
                return true;
            }
            _currentStreamIndex++;
        }
        return false;
    }
    size_t LidarStreamerComposite::nPoints() const
    {
        size_t total = 0;
        for (const auto& stream : _streams) {
            total += stream->nPoints();
        }
        return total;
    }
    size_t LidarStreamerComposite::nPointsRemaining() const
    {
        size_t total = 0;
        for (size_t i = _currentStreamIndex; i < _streams.size(); i++) {
            total += _streams[i]->nPointsRemaining();
        }
        return total;
    }
    const CoordRef& LidarStreamerComposite::getCoordRef() const
    {
        return _crs;
    }
    const Extent& LidarStreamerComposite::getExtent() const
    {
        return _extent;
    }
    void LidarStreamerComposite::reset()
    {
        for (auto& stream : _streams) {
            stream->reset();
        }
        _currentStreamIndex = 0;
    }
    bool LidarStreamerComposite::_canModifyInPlace() const
    {
        return true;
    }
}