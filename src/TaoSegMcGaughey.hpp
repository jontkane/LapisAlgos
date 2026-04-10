#pragma once
#ifndef TAOSEGMCGAUGHEY_HPP
#define TAOSEGMCGAUGHEY_HPP

#include"algos_pch.hpp"
#include"TaoSeg.hpp"

namespace lapis {

    enum class McGaugheySmoothType {
        fusion,
        simple,
        none
    };

    class TaoSegMcGaughey : public TaoSeg {
    public:
        TaoSegMcGaughey(int nVertices,
            csm_t slopeChangeMultiplier,
            csm_t heightCutoffMultiplier,
            coord_t maxDistMultiplier,
            McGaugheySmoothType smoothType);

        static TaoSegMcGaughey fusionDefaults();

        SegmentResults process(const Raster<csm_t>& bufferedCsm, const Extent& unbufferedExtent, const std::vector<IDedTao>& taos) const override;
    private:
        int _nVertices;
        csm_t _slopeChangeMultiplier;
        csm_t _heightCutoffMultiplier;
        coord_t _maxDistMultiplier;
        McGaugheySmoothType _smoothType;

        Polygon _processOneTao(const coord_t x, const coord_t y, const Raster<csm_t>& bufferedCsm) const;
        void _fusionSmooth(std::vector<coord_t>& vertexDistances) const;
        void _simpleSmooth(std::vector<coord_t>& vertexDistances) const;
    };
}

#endif