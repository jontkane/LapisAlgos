#pragma once
#ifndef LIDARSTREAMERMINMAXFILTER_HPP
#define LIDARSTREAMERMINMAXFILTER_HPP

#include"algos_pch.hpp"
#include"LidarStreamer.hpp"

namespace lapis {
    class LidarStreamerMinMaxFilter : public LidarStreamer {
    public:
        LidarStreamerMinMaxFilter(std::unique_ptr<LidarStreamer> streamer, coord_t minZ, coord_t maxZ);
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
        std::unique_ptr<LidarStreamer> _streamer;
        coord_t _minZ, _maxZ;
        std::vector<LasPoint> _buffer;
    };
}

#endif