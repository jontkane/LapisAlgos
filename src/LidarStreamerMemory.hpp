#pragma once
#ifndef LIDARSTREAMERMEMORY_HPP
#define LIDARSTREAMERMEMORY_HPP

#include"algos_pch.hpp"
#include"LidarStreamer.hpp"

namespace lapis {
    class LidarStreamerMemory : public LidarStreamer {
    public:
        LidarStreamerMemory(const std::vector<LasPoint>& points, const CoordRef& crs, const Extent& e);
        LidarStreamerMemory(std::vector<LasPoint>&& points, const CoordRef& crs, const Extent& e);
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
        std::vector<LasPoint> _points;
        size_t _currentIndex;
        CoordRef _crs;
        Extent _extent;
    };
}

#endif