#pragma once
#ifndef TAOIDHIGHPOINTS_HPP
#define TAOIDHIGHPOINTS_HPP

#include"algos_pch.hpp"
#include"TaoId.hpp"

namespace lapis {
    class TaoIdHighPoints : public TaoIdAlgo {
    public:
        TaoIdHighPoints(coord_t minHtCsmZUnits, coord_t minDistCsmXYUnits);
        std::vector<IDedTao> process(const Raster<csm_t>& bufferedCsm, TaoIdGenerator idGen) const override;
    private:
        coord_t _minHt;
        coord_t _minDist;
        std::vector<IDedTao> _taoCandidates(const Raster<csm_t>& bufferedCsm, TaoIdGenerator& idGen) const;
    };
}

#endif