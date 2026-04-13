#include "TaoSegWatershed.hpp"

namespace lapis {
	//this data structure is taken from https://www.researchgate.net/publication/261191274_Hierarchical_Queues_general_description_and_implementation_in_MAMBA_Image_library
	class HierarchicalQueue {
	public:
		HierarchicalQueue(csm_t min, csm_t max, csm_t binSize);

		void push(csm_t height, cell_t cell);

		//pops the next element of the queue and returns it, or returns -1 if the queue is empty
		cell_t popAndReturn();

		size_t size() const;

	private:
		csm_t min, max, binSize;
		size_t currentQueue;
		std::vector<std::queue<cell_t>> queues;
		size_t _size;
	};

	size_t HierarchicalQueue::size() const {
		return _size;
	}

	HierarchicalQueue::HierarchicalQueue(csm_t min, csm_t max, csm_t binSize) {
		this->min = min;
		this->max = max;
		this->binSize = binSize;
		currentQueue = 0;
		size_t nBins = (size_t)std::ceil((max - min) / binSize);
		queues = std::vector<std::queue<cell_t>>(nBins);
		_size = 0;
	}

	void HierarchicalQueue::push(csm_t height, cell_t cell) {
		height = std::min(height, max);
		size_t bin = (size_t)((max - height) / binSize);
		bin = std::max(bin, currentQueue);
		bin = std::min(bin, queues.size() - 1); //this is needed if the value is exactly equal to min
		queues[bin].push(cell);
		++_size;
	}

	cell_t HierarchicalQueue::popAndReturn() {
		if (_size == 0) {
			return -1;
		}
		while (!queues[currentQueue].size()) {
			currentQueue++;
		}
		cell_t out = queues[currentQueue].front();
		queues[currentQueue].pop();
		--_size;
		return out;
	}

	TaoSegWatershed::TaoSegWatershed(csm_t minHt, csm_t maxHt)
		: _minHt(minHt), _maxHt(maxHt)
	{}
	TaoSegAlgo::SegmentResults TaoSegWatershed::process(const Raster<csm_t>& bufferedCsm, const Extent& unbufferedExtent, const std::vector<IDedTao>& taos) const
	{
		//this is modified from https://arxiv.org/pdf/1511.04463.pdf
		//algorithm 5 on page 15
		HierarchicalQueue open{ _minHt, _maxHt, _binSize };
		constexpr taoid_t TO_BE_LABELED = -1;
		Raster<taoid_t> labels((Alignment)bufferedCsm);
		for (cell_t cell = 0; cell < labels.ncell(); ++cell) {
			if (bufferedCsm.atCellUnsafe(cell).has_value() && bufferedCsm.atCellUnsafe(cell).value() >= _minHt) {
				labels.atCellUnsafe(cell).has_value() = true;
				labels.atCellUnsafe(cell).value() = TO_BE_LABELED;
			}
		}

		for (IDedTao tao : taos) {
            cell_t cell = tao.location;
			if (cell < 0 || cell >= labels.ncell()) {
				continue;
			}
            auto v = bufferedCsm.atCellUnsafe(cell);
			if (!v.has_value() || v.value() < _minHt) {
				continue;
			}
			labels.atCellUnsafe(tao.location).value() = tao.id;
			open.push(bufferedCsm.atCellUnsafe(tao.location).value(), tao.location);
		}

		while (open.size()) {
			cell_t c = open.popAndReturn();

			rowcol_t row = labels.rowFromCellUnsafe(c);
			rowcol_t col = labels.colFromCellUnsafe(c);

			struct rc { rowcol_t row, col; };
			std::array<rc, 8> neighbors = { { {row + 1,col},{row - 1,col},{row,col + 1},{row,col - 1},
				{row + 1,col + 1},{row - 1,col - 1},{row + 1,col - 1},{row - 1,col + 1}
			} };

			for (auto& thisRC : neighbors) {
				rowcol_t rowNudge = thisRC.row;
				rowcol_t colNudge = thisRC.col;
				if (rowNudge < 0 || colNudge < 0 || rowNudge >= bufferedCsm.nrow() || colNudge >= bufferedCsm.ncol()) {
					continue;
				}
				cell_t n = bufferedCsm.cellFromRowColUnsafe(rowNudge, colNudge);
				if (!labels.atCellUnsafe(n).has_value() || labels.atCellUnsafe(n).value() != TO_BE_LABELED) {
					continue;
				}
				labels[n].value() = labels[c].value();
				//the original algorithm had an optimization here using a regular queue but that was only an optimization
				//if the cells you started from were kind of arbitrary
				//by handpicking high points, it's unneccesary, and comes with a bit of overhead as well
				open.push(bufferedCsm.atCellUnsafe(n).value(), n);
			}
		}

		std::unordered_set<taoid_t> idsToNa;
		for (IDedTao tao : taos) {
			coord_t x = bufferedCsm.xFromCellUnsafe(tao.location);
			coord_t y = bufferedCsm.yFromCellUnsafe(tao.location);
			if (!unbufferedExtent.contains(x, y)) {
				//this tao has its stem outside the extent of the tile; it's another tile's problem
				idsToNa.insert(tao.id);
			}
		}

		for (cell_t cell : CellIterator(labels)) {
			auto v = labels.atCellUnsafe(cell);
			if (!v.has_value()) {
				continue;
			}
			if (idsToNa.contains(v.value())) {
				v.has_value() = false;
			}
		}

		SegmentResults out;
		out.segmentRaster = std::move(labels);
		out.segmentPolygons = rasterToMultiPolygonForTaos(*out.segmentRaster, nullptr);

		return out;
	}
	std::string TaoSegWatershed::name()
	{
		return "Watershed";
	}
}