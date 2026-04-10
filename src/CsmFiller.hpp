#pragma once
#ifndef CSMFILLER_HPP
#define CSMFILLER_HPP

#include"algos_pch.hpp"
#include"CsmMaker.hpp"

namespace lapis {
    class CsmFiller : public CsmMaker {
    public:
        CsmFiller(std::unique_ptr<CsmMaker> csmMaker, int neighborsNeeded, coord_t lookDistCsmXYUnits);
        Raster<csm_t> getRaster() const override;
        void addPoints(const std::span<const LasPoint>& points) override;
        void addPointsUnsafe(const std::span<const LasPoint>& points) override;
        CsmMergeFunc getMergeFunction() const override;
    private:
        std::unique_ptr<CsmMaker> _csmMaker;
        int _neighborsNeeded;
        coord_t _lookDistCsmXYUnits;
    };
}

#endif