#pragma once
#ifndef LIDARSTREAMERHARDDRIVE_HPP
#define LIDARSTREAMERHARDDRIVE_HPP

#include"LidarStreamer.hpp"

namespace lapis {
    class LidarStreamerHardDrive : public LidarStreamer {
    public:
        LidarStreamerHardDrive(const std::string& filename);
        LidarStreamerHardDrive(const std::filesystem::path& filepath);
        LidarStreamerHardDrive(LasReader&& reader);
        const std::span<const LasPoint> getPoints(size_t n) override;
        bool hasMorePoints() const override;
        size_t nPoints() const override;
        size_t nPointsRemaining() const override;
        const CoordRef& getCoordRef() const override;
        const Extent& getExtent() const override;
        void reset() override;
    protected:
        bool _canModifyInPlace() const override;
    private:
        LasReader _reader;
        LidarPointVector _buffer;
    };
}

#endif