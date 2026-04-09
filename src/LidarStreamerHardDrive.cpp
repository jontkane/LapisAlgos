#include"LidarStreamerHardDrive.hpp"

namespace lapis {
    LidarStreamerHardDrive::LidarStreamerHardDrive(const std::string& filename) : _reader(filename) {}
    LidarStreamerHardDrive::LidarStreamerHardDrive(const std::filesystem::path& filepath) : _reader(filepath.string()) {}
    LidarStreamerHardDrive::LidarStreamerHardDrive(LasReader&& reader)
        : _reader(std::move(reader))
    {}
    const std::span<const LasPoint> LidarStreamerHardDrive::getPoints(size_t n) {
        _buffer = _reader.getPoints(n);
        return _buffer;
    }
    bool LidarStreamerHardDrive::hasMorePoints() const {
        return _reader.nPointsRemaining() > 0;
    }
    size_t LidarStreamerHardDrive::nPoints() const
    {
        return _reader.nPoints();
    }
    size_t LidarStreamerHardDrive::nPointsRemaining() const
    {
        return _reader.nPointsRemaining();
    }
    const CoordRef& LidarStreamerHardDrive::getCoordRef() const {
        return _reader.crs();
    }
    const Extent& LidarStreamerHardDrive::getExtent() const
    {
        return _reader;
    }
    void LidarStreamerHardDrive::reset()
    {
        _reader.reset();
    }
    bool LidarStreamerHardDrive::_canModifyInPlace() const
    {
        return true;
    }
}