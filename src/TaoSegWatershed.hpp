#pragma once
#ifndef TAOSEGWATERSHED_HPP
#define TAOSEGWATERSHED_HPP

#include"algos_pch.hpp"
#include"TaoSeg.hpp"

namespace lapis {
    class TaoSegWatershed : public TaoSegAlgo {
    public:
        TaoSegWatershed(csm_t minHt, csm_t maxHt);
        SegmentResults process(const Raster<csm_t>& bufferedCsm, const Extent& unbufferedExtent, const std::vector<IDedTao>& taos) const override;

        static std::string name();
    private:
        csm_t _minHt;
        csm_t _maxHt;
        constexpr static csm_t _binSize = 0.01;
    };
}

#endif