#include"CsmPreMade.hpp"

namespace lapis {
    CsmPreMade::CsmPreMade(const Raster<csm_t>& raster, CsmMergeFunc mergeFunc) : _raster(raster), _mergeFunc(mergeFunc) {}
    CsmPreMade::CsmPreMade(const Raster<csm_t>&& raster, CsmMergeFunc mergeFunc) : _raster(std::move(raster)), _mergeFunc(mergeFunc) {}
    Raster<csm_t> CsmPreMade::getRaster() const {
        return _raster;
    }
    void CsmPreMade::addPoints(const std::span<const LasPoint>& points) {
        //intentionally empty
    }
    CsmMaker::CsmMergeFunc CsmPreMade::getMergeFunction() const
    {
        return _mergeFunc;
    }
}