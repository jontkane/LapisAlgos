#include"CsmSmoother.hpp"

namespace lapis {
    CsmSmoother::CsmSmoother(std::unique_ptr<CsmMaker> csm, int smoothWindow)
        : _csm(std::move(csm)), _smoothWindow(smoothWindow)
    {
        if (smoothWindow < 1) {
            throw std::invalid_argument("Smooth window must be at least 1");
        }
        if (_smoothWindow % 2 == 0) {
            throw std::invalid_argument("Smooth window must be odd");
        }
    }
    Raster<csm_t> CsmSmoother::getRaster() const
    {
        Raster<csm_t> r = _csm->getRaster();
        rowcol_t lookDist = _smoothWindow / 2;
        for (cell_t cell : CellIterator(r)) {
            auto v = r[cell];
            if (!v.has_value()) {
                continue;  
            }
            csm_t sum = 0;
            csm_t count = 0;
            rowcol_t row = r.rowFromCellUnsafe(cell);
            rowcol_t col = r.colFromCellUnsafe(cell);
            for (rowcol_t rowBudge = -lookDist; rowBudge <= lookDist; ++rowBudge) {
                rowcol_t neighborRow = row + rowBudge;
                if (neighborRow < 0 || neighborRow >= r.nrow()) {
                    continue;  
                }
                for (rowcol_t colBudge = -lookDist; colBudge <= lookDist; ++colBudge) {
                    rowcol_t neighborCol = col + colBudge;
                    if (neighborCol < 0 || neighborCol >= r.ncol()) {
                        continue;  
                    }
                    auto neighborV = r.atRCUnsafe(neighborRow, neighborCol);
                    if (!neighborV.has_value()) {
                        continue;  
                    }
                    sum += neighborV.value();
                    count += 1;
                }
            }
            v.value() = sum / count;
        }
        return r;
    }
    void CsmSmoother::addPoints(const std::span<const LasPoint>& points)
    {
        _csm->addPoints(points);
    }
    void CsmSmoother::addPointsUnsafe(const std::span<const LasPoint>& points)
    {
        _csm->addPointsUnsafe(points);
    }
    CsmMaker::CsmMergeFunc CsmSmoother::getMergeFunction() const
    {
        return CsmMaker::mergeByMean;
    }
}