#include"TaoIdHighPoints.hpp"

namespace lapis {
    TaoIdHighPoints::TaoIdHighPoints(coord_t minHtCsmZUnits, coord_t minDistCsmXYUnits)
        : _minHt(minHtCsmZUnits), _minDist(minDistCsmXYUnits)
    {}
    std::vector<IDedTao> TaoIdHighPoints::process(const Raster<csm_t>& bufferedCsm, TaoIdGenerator idGen) const
    {
		std::vector<IDedTao> candidates = _taoCandidates(bufferedCsm, idGen);

		if (_minDist <= 0) {
			return candidates;
		}

		struct SortableCandidate {
			csm_t value;
			cell_t cell;
			taoid_t id;
		};
		std::vector<SortableCandidate> sortableValues;
		sortableValues.reserve(candidates.size());

		for (IDedTao tao : candidates) {
			auto v = bufferedCsm.atCellUnsafe(tao.location);
			sortableValues.emplace_back(v.value(), tao.location, tao.id);
		}
		std::sort(sortableValues.begin(), sortableValues.end(), [](auto& a, auto& b) {return a.value > b.value; });

		Raster<bool> masked{ (Alignment)bufferedCsm }; //this raster's values will be true for pixels which don't qualify as high points because they're too close to another high point

		struct RelativePosition {
			rowcol_t x, y;
		};
		std::vector<RelativePosition> circle;

		//this assumes square pixels, but the CSMs output by this library will always be square
		rowcol_t maxPixels = (rowcol_t)std::ceil(std::abs(_minDist / bufferedCsm.xres()));
		circle.reserve((size_t)maxPixels * maxPixels);

		coord_t minDistPixSq = (_minDist / bufferedCsm.xres()) * (_minDist / bufferedCsm.xres());
		for (rowcol_t r = -maxPixels; r <= maxPixels; ++r) {
			for (rowcol_t c = -maxPixels; c <= maxPixels; ++c) {
				if (r * r + c * c < minDistPixSq) {
					circle.emplace_back(c, r);
				}
			}
		}

		std::vector<IDedTao> out;

		for (SortableCandidate& candidate : sortableValues) {
			if (!masked[candidate.cell].value()) {
				out.push_back(IDedTao{ candidate.id, candidate.cell });

				rowcol_t thisRow = masked.rowFromCellUnsafe(candidate.cell);
				rowcol_t thisCol = masked.colFromCellUnsafe(candidate.cell);
				for (RelativePosition& rp : circle) {
					rowcol_t row = thisRow + rp.y;
					rowcol_t col = thisCol + rp.x;
					if (row < 0 || row >= masked.nrow() || col < 0 || col >= masked.ncol()) {
						continue;
					}
					masked.atRCUnsafe(row, col).value() = true;
				}
			}
		}
		return out;
    }
	std::vector<IDedTao> TaoIdHighPoints::_taoCandidates(const Raster<csm_t>& bufferedCsm, TaoIdGenerator& idGen) const
	{
		std::vector<IDedTao> candidates;
		candidates.reserve(bufferedCsm.ncell() / 10);

		for (rowcol_t row = 0; row < bufferedCsm.nrow(); ++row) {
			for (rowcol_t col = 0; col < bufferedCsm.ncol(); ++col) {
				auto center = bufferedCsm.atRCUnsafe(row, col);
				if (!center.has_value()) {
					continue;
				}
				if (center.value() < _minHt) {
					continue;
				}
				bool isHighPoint = true;

				//this setup with priority is to ensure that areas with strictly equal height only get one candidate
				struct rc { rowcol_t row, col; };
				std::vector<rc> higherPriorityNeighbors = { {row + 1,col},{row,col + 1},{row + 1,col + 1},{row - 1,col + 1} };
				std::vector<rc> lowerPriorityNeighbors = { {row - 1,col},{row,col - 1},{row - 1,col - 1},{row + 1,col - 1} };

				for (auto& thisRC : higherPriorityNeighbors) {
					rowcol_t rowNudge = thisRC.row;
					rowcol_t colNudge = thisRC.col;
					if (rowNudge < 0 || colNudge < 0 || rowNudge >= bufferedCsm.nrow() || colNudge >= bufferedCsm.ncol()) {
						continue;
					}
					auto compare = bufferedCsm.atRCUnsafe(rowNudge, colNudge);
					if (!compare.has_value()) {
						continue;
					}
					if (compare.value() > center.value()) {
						isHighPoint = false;
						break;
					}
				}
				if (isHighPoint) {
					for (auto& thisRC : lowerPriorityNeighbors) {
						rowcol_t rowNudge = thisRC.row;
						rowcol_t colNudge = thisRC.col;
						if (rowNudge < 0 || colNudge < 0 || rowNudge >= bufferedCsm.nrow() || colNudge >= bufferedCsm.ncol()) {
							continue;
						}
						auto compare = bufferedCsm.atRCUnsafe(rowNudge, colNudge);
						if (!compare.has_value()) {
							continue;
						}
						if (compare.value() >= center.value()) {
							isHighPoint = false;
							break;
						}
					}
				}

				if (isHighPoint) {
					candidates.push_back(IDedTao{ idGen.nextId(), bufferedCsm.cellFromRowColUnsafe(row, col) });
				}
			}
		}
		return candidates;
	}
}