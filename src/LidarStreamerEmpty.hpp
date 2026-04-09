#pragma once
#ifndef LIDARSTREAMEREMPTY_HPP
#define LIDARSTREAMEREMPTY_HPP

#include"algos_pch.hpp"
#include"LidarStreamer.hpp"

namespace lapis {
    class LidarStreamerEmpty : public LidarStreamer {
    public:
        LidarStreamerEmpty() = default;
        LidarStreamerEmpty(const CoordRef& crs, const Extent& extent);
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
        Extent _extent;
    };
}

#endif