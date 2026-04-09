#include"CsmFiller.hpp"

namespace lapis {
    CsmFiller::CsmFiller(std::unique_ptr<CsmMaker> csmMaker, int neighborsNeeded, coord_t lookDistCsmXYUnits)
        : _csmMaker(std::move(csmMaker)), _neighborsNeeded(neighborsNeeded), _lookDistCsmXYUnits(lookDistCsmXYUnits) {}
    Raster<csm_t> CsmFiller::getRaster() const
    {
        Raster<csm_t> original = _csmMaker->getRaster();
        Raster<csm_t> out{ original };

		int maxMisses = 8 - _neighborsNeeded;
		coord_t xres = original.xres();
		coord_t yres = original.yres();
        coord_t diagonal = std::sqrt(xres * xres + yres * yres);
		rowcol_t northSouthPixelDist = static_cast<rowcol_t>(std::ceil(_lookDistCsmXYUnits / yres));
		rowcol_t eastWestPixelDist = static_cast<rowcol_t>(std::ceil(_lookDistCsmXYUnits / xres));
		rowcol_t diagonalFillDist = static_cast<rowcol_t>(std::ceil(_lookDistCsmXYUnits / diagonal));

		struct Direction {
			rowcol_t rowDir, colDir;
		};
		static std::vector<Direction> directions = {
			{0,1},{0,-1},{1,0},{-1,0},
			{1,1},{1,-1},{-1,1},{-1,-1}
		};
		std::vector<rowcol_t> maxPixelDists = {
			eastWestPixelDist, eastWestPixelDist, northSouthPixelDist, northSouthPixelDist,
			diagonalFillDist, diagonalFillDist, diagonalFillDist, diagonalFillDist
		};
		std::vector<coord_t> distMultipliers = {
			xres, xres, yres, yres,
			diagonal, diagonal, diagonal, diagonal
		};

        for (cell_t cell : CellIterator(out)) {
            auto origV = original.atCellUnsafe(cell);
            if (origV.has_value()) {
                continue;
            }

			//the algorithm here to distinguish legitimately absent data from holes is:
			//in all 8 cardinal directions, look for data up to a certain distance away
			//if you find data or the edge of the raster in at least neighborsneeded directions,
			//then assign the value to be an inverse-distance-weighted mean of the closest value found in each direction


			struct ValueDist {
				csm_t value;
				csm_t dist;
			};
			std::vector<ValueDist> foundValues;
			foundValues.reserve(8);

			int missesSoFar = 0;

			rowcol_t row = original.rowFromCell(cell);
			rowcol_t col = original.colFromCell(cell);

			for (size_t i = 0; i < directions.size(); ++i) {

                rowcol_t maxPixelDist = maxPixelDists[i];

				bool missed = true;

				for (rowcol_t d = 1; d <= maxPixelDist; ++d) {
					rowcol_t thisRow = row + d * directions[i].rowDir;
					rowcol_t thisCol = col + d * directions[i].colDir;
					if (thisRow < 0 || thisCol < 0 || thisRow >= original.nrow() || thisCol >= original.ncol()) {
						//in this case, we found the edge of the raster
						//we don't have a value to contribute to the eventual mean, but we don't count it as a miss either
						missed = false;
						break;
					}
					auto v = original.atRCUnsafe(thisRow, thisCol);
					if (!v.has_value()) {
						continue;
					}
					missed = false;
					foundValues.emplace_back(v.value(), distMultipliers[i] * d);
					break;
				}
				if (missed) {
					missesSoFar++;
					if (missesSoFar > maxMisses) {
						break;
					}
				}
			}

			csm_t numerator = 0;
			csm_t denominator = 0;
			for (const ValueDist v : foundValues) {
				csm_t inverseDist = 1 / v.dist;
				numerator += inverseDist * v.value;
				denominator += inverseDist;
			}
			auto v = out.atCellUnsafe(cell);
			if (denominator != 0) {
				v.has_value() = true;
				v.value() = numerator / denominator;
			}
        }

        return out;
    }
    void CsmFiller::addPoints(const std::span<const LasPoint>& points)
    {
        _csmMaker->addPoints(points);
    }
	CsmMaker::CsmMergeFunc CsmFiller::getMergeFunction() const
	{
        return _csmMaker->getMergeFunction();
	}
}