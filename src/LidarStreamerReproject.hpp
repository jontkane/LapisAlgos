#pragma once
#ifndef LIDARSTREAMERREPROJECT_HPP
#define LIDARSTREAMERREPROJECT_HPP

#include"algos_pch.hpp"
#include"LidarStreamer.hpp"

namespace lapis {
    class LidarStreamerReproject : public LidarStreamer {
    public:
        LidarStreamerReproject(std::unique_ptr<LidarStreamer>&& source, const CoordRef& targetCRS);
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
        std::vector<LasPoint> _buffer;
        std::unique_ptr<LidarStreamer> _source;
        const CoordTransform* _tr;
        Extent _e;
    };
}

#endif