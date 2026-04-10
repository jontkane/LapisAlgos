#pragma once
#ifndef TAOSEG_HPP
#define TAOSEG_HPP

#include"algos_pch.hpp"
#include"algos_types.hpp"

namespace lapis {
    class TaoSeg {
    public:
        virtual ~TaoSeg() noexcept = default;

        struct SegmentResults {
            std::optional<Raster<taoid_t>> segmentRaster;
            std::optional<VectorDataset<MultiPolygon>> segmentPolygons;
        };
        virtual SegmentResults process(const Raster<csm_t>& bufferedCsm, const Extent& unbufferedExtent, const std::vector<IDedTao>& taos) const = 0;
    };
}

#endif