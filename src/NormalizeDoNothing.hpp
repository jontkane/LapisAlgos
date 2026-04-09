#pragma once
#ifndef NORMALIZEDONOTHING_HPP
#define NORMALIZEDONOTHING_HPP

#include"algos_pch.hpp"
#include"LidarStreamer.hpp"

namespace lapis {
    class NormalizeDoNothing : public LidarStreamer {
    public:
        NormalizeDoNothing(std::unique_ptr<LidarStreamer> streamer);
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
        std::unique_ptr<LidarStreamer> _source;
    };
}

#endif