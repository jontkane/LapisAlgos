#include"CsmMaxHeight.hpp"

namespace lapis {
    CsmMaxHeight::CsmMaxHeight(const Alignment& a, coord_t footprintDiameterCsmXYUnits)
        : _footprintRadius(footprintDiameterCsmXYUnits / 2.)
    {
		Alignment buffered = Alignment{ bufferExtent(a, _footprintRadius * 1.01), a.xOrigin(), a.yOrigin(), a.xres(), a.yres() };
        _r = Raster<csm_t>{ buffered };
		for (cell_t cell : CellIterator(_r)) {
			auto v = _r.atCellUnsafe(cell);
            v.value() = std::numeric_limits<csm_t>::lowest();
        }

		const coord_t diagonal = _footprintRadius / std::sqrt(2);
		const coord_t epsilon = -0.000001; //the purpose of this value is to avoid ties caused by the footprint algorithm messing with the high points TAO algorithm

		if (_footprintRadius == 0) {
			_circle = { {0.,0.,0.} };
		}
		else {
			_circle = { {0,0,0},
				{_footprintRadius,0,epsilon},
				{-_footprintRadius,0,epsilon},
				{0,_footprintRadius,epsilon},
				{0,-_footprintRadius,epsilon},
                //diagonals get double epsilon to make sure they don't cause ties with the cardinal directions
				{diagonal,diagonal,2 * epsilon},
				{diagonal,-diagonal,2 * epsilon},
				{-diagonal,diagonal,2 * epsilon},
				{-diagonal,-diagonal,2 * epsilon} };
		}
	}
    Raster<csm_t> CsmMaxHeight::getRaster() const
    {
		for (cell_t cell : CellIterator(_r)) {
			auto v = _r.atCellUnsafe(cell);
			if (v.value() > std::numeric_limits<csm_t>::lowest()) {
				v.has_value() = true;
            }
		}
        return _r;
    }
    void CsmMaxHeight::addPoints(const std::span<const LasPoint>& points)
    {

		if (_footprintRadius == 0) {
			for (const LasPoint& p : points) {
				if (!_r.contains(p.x, p.y)) {
					continue;
                }
				cell_t cell = _r.cellFromXYUnsafe(p.x, p.y);
				auto v = _r.atCellUnsafe(cell);
				v.value() = std::max(v.value(), p.z);
			}
            return;
		}

		for (const LasPoint& p : points) {
			for (const XYEpsilon& direction : _circle) {
				coord_t x = p.x + direction.x;
				coord_t y = p.y + direction.y;
				if (!_r.contains(x, y)) {
					continue;
                }
				csm_t z = p.z + direction.epsilon;
				cell_t cell = _r.cellFromXYUnsafe(x, y);
                auto v = _r.atCellUnsafe(cell);
				v.value() = std::max(v.value(), z);
			}
		}
    }
	void CsmMaxHeight::addPointsUnsafe(const std::span<const LasPoint>& points)
	{
		if (_footprintRadius == 0) {
			for (const LasPoint& p : points) {
                assert(_r.contains(p.x, p.y));
				cell_t cell = _r.cellFromXYUnsafe(p.x, p.y);
				auto v = _r.atCellUnsafe(cell);
				v.value() = std::max(v.value(), p.z);
			}
			return;
		}
		for (const LasPoint& p : points) {
			for (const XYEpsilon& direction : _circle) {
				coord_t x = p.x + direction.x;
				coord_t y = p.y + direction.y;
				assert(_r.contains(x, y));
				csm_t z = p.z + direction.epsilon;
				cell_t cell = _r.cellFromXYUnsafe(x, y);
				auto v = _r.atCellUnsafe(cell);
				v.value() = std::max(v.value(), z);
			}
        }
	}
    CsmMaker::CsmMergeFunc CsmMaxHeight::getMergeFunction() const
    {
        return CsmMaker::mergeByMax;
    }
}