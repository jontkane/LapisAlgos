#pragma once
#ifndef LIDARSTREAMERMULTIFILE_HPP
#define LIDARSTREAMERMULTIFILE_HPP

#include"algos_pch.hpp"
#include"LidarStreamer.hpp"
#include"LidarStreamerHardDrive.hpp"

namespace lapis {
    class LidarStreamerComposite : public LidarStreamer {
    public:
        LidarStreamerComposite() = default;
        LidarStreamerComposite(const CoordRef& crs);

        void addStream(std::unique_ptr<LidarStreamer> stream);

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
        CoordRef _crs;
        Extent _extent{};
        std::vector<LasPoint> _buffer;
        std::vector<std::unique_ptr<LidarStreamer>> _streams;
        mutable size_t _currentStreamIndex = 0;
    };
}

#endif