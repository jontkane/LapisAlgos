#pragma once
#ifndef CSMPREMADE_HPP
#define CSMPREMADE_HPP

#include"algos_pch.hpp"
#include"CsmMaker.hpp"

namespace lapis {
    class CsmPreMade : public CsmMaker {
    public:
        CsmPreMade(const Raster<csm_t>& raster, CsmMergeFunc mergeFunc = CsmMaker::mergeByMean);
        CsmPreMade(Raster<csm_t>&& raster, CsmMergeFunc mergeFunc = CsmMaker::mergeByMean);
        Raster<csm_t> getRaster() const override;
        void addPoints(const std::span<const LasPoint>& points) override;
        void addPointsUnsafe(const std::span<const LasPoint>& points) override;
        CsmMergeFunc getMergeFunction() const override;
    private:
        Raster<csm_t> _raster;
        CsmMergeFunc _mergeFunc;
    };
}

#endif