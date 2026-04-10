#pragma once
#ifndef CSMMAXHEIGHT_HPP
#define CSMMAXHEIGHT_HPP

#include"algos_pch.hpp"
#include"CsmMaker.hpp"

namespace lapis {
    class CsmMaxHeight : public CsmMaker {
    public:
        CsmMaxHeight(const Alignment& a, coord_t footprintDiameterCsmXYUnits = 0);
        Raster<csm_t> getRaster() const override;
        void addPoints(const std::span<const LasPoint>& points) override;
        void addPointsUnsafe(const std::span<const LasPoint>& points) override;
        CsmMergeFunc getMergeFunction() const override;
    private:
        Raster<csm_t> _r;
        coord_t _footprintRadius;

        struct XYEpsilon {
            coord_t x, y, epsilon;
        };
        std::vector<XYEpsilon> _circle;
    };
}

#endif