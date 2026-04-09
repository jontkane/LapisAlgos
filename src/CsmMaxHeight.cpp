#include"CsmMaxHeight.hpp"

namespace lapis {
    CsmMaxHeight::CsmMaxHeight(const Alignment& a)
        : _r(a)
    {}
    Raster<csm_t> CsmMaxHeight::getRaster() const
    {
        return _r;
    }
    void CsmMaxHeight::addPoints(const std::span<const LasPoint>& points)
    {
        for (const auto& p : points) {
            if (_r.contains(p.x, p.y)) {
                auto cell = _r.atXYUnsafe(p.x, p.y);
                if (!cell.has_value()) {
                    cell.has_value() = true;
                    cell.value() = p.z;
                }
                else if (p.z > cell.value()) {
                    cell.value() = p.z;
                }
            }
        }
    }
    CsmMaker::CsmMergeFunc CsmMaxHeight::getMergeFunction() const
    {
        return CsmMaker::mergeByMax;
    }
}