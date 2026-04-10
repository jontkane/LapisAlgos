#include"TaoSegMcGaughey.hpp"

namespace lapis {
    TaoSegMcGaughey::TaoSegMcGaughey(int nVertices, csm_t slopeChangeMultiplier, csm_t heightCutoffMultiplier, coord_t maxDistMultiplier, McGaugheySmoothType smoothType)
        : _nVertices(nVertices), _slopeChangeMultiplier(slopeChangeMultiplier), 
        _heightCutoffMultiplier(heightCutoffMultiplier), _maxDistMultiplier(maxDistMultiplier), 
        _smoothType(smoothType) {
		if (nVertices < 3) {
			throw std::invalid_argument("nVertices must be at least 3.");
        }
		if (slopeChangeMultiplier < 0) {
			throw std::invalid_argument("slopeChangeMultiplier must be non-negative.");
        }
        if (heightCutoffMultiplier < 0) {
			throw std::invalid_argument("heightCutoffMultiplier must be non-negative.");
		}
		if (maxDistMultiplier < 0) {
			throw std::invalid_argument("maxDistMultiplier must be non-negative.");
		}
	}
    TaoSegMcGaughey TaoSegMcGaughey::fusionDefaults()
    {
        return TaoSegMcGaughey(16, 4, 2./3., 3./4., McGaugheySmoothType::fusion);
    }
    TaoSeg::SegmentResults TaoSegMcGaughey::process(const Raster<csm_t>& bufferedCsm, const Extent& unbufferedExtent, const std::vector<IDedTao>& taos) const
    {
        VectorDataset<MultiPolygon> polygons{ bufferedCsm.crs() };
        polygons.addNumericField<taoid_t>("ID");
        polygons.reserve(taos.size());
        for (IDedTao tao : taos) {
            coord_t x = bufferedCsm.xFromCellUnsafe(tao.location);
            coord_t y = bufferedCsm.yFromCellUnsafe(tao.location);
            if (!unbufferedExtent.contains(x, y)) {
                continue;
            }
            try {
                Polygon poly = _processOneTao(x, y, bufferedCsm);
                MultiPolygon mp;
                mp.addPolygon(poly);
                polygons.addGeometry(mp);
                polygons.back().setNumericField<taoid_t>("ID", tao.id);
            }
            catch (...) {} //the possible exceptions only occur if the input is invalid, but if they do, we just skip this tao

        }

        SegmentResults results;
        results.segmentPolygons = std::move(polygons);
        return results;
    }
    Polygon TaoSegMcGaughey::_processOneTao(const coord_t x, const coord_t y, const Raster<csm_t>& bufferedCsm) const
    {
		if (!bufferedCsm.contains(x, y)) {
			throw std::runtime_error("Tao location is outside of the raster bounds.");
		}
		auto v = bufferedCsm.atXYUnsafe(x, y);
		if (!v.has_value()) {
			throw std::runtime_error("Tao location has no value in the raster.");
		}
		csm_t height = v.value();
		if (height <= 0) {
			throw std::runtime_error("Tao location has non-positive height.");
		}

		double angleSpacing = 2. * M_PI / _nVertices;
		constexpr double sqrtTwo = std::numbers::sqrt2;
		constexpr int maxPoints = 1023;

		const csm_t heightCutoff = height * _heightCutoffMultiplier;
		const coord_t maxDist = height * _maxDistMultiplier;
		const int nPoints = std::max(1, std::min(maxPoints, (int)(maxDist * sqrtTwo / bufferedCsm.xres())));
		const coord_t pointSpacing = maxDist / nPoints;

		std::vector<coord_t> vertexDistances;
		vertexDistances.reserve(_nVertices);

		std::vector<coord_t> vertexSins;
        vertexSins.reserve(_nVertices);
        std::vector<coord_t> vertexCoss;
        vertexCoss.reserve(_nVertices);
		for (size_t vertex = 0; vertex < _nVertices; ++vertex) {
			vertexSins.push_back(std::sin(angleSpacing * vertex));
			vertexCoss.push_back(std::cos(angleSpacing * vertex));
        }


		for (int vertexIdx = 0; vertexIdx < _nVertices; ++vertexIdx) {

			auto getPoint = [&](int spacingCount) {
				return CoordXY{ x + spacingCount * vertexCoss[vertexIdx] * pointSpacing ,y + spacingCount * vertexSins[vertexIdx] * pointSpacing };
				};


			int currentSpacing = 1;
			CoordXY prevPoint{ x,y };
			xtl::xoptional<csm_t> prevHeight = height;
			CoordXY currentPoint = getPoint(currentSpacing);
			xtl::xoptional<csm_t> currentHeight = bufferedCsm.extract(currentPoint.x, currentPoint.y, ExtractMethod::bilinear);
			CoordXY nextPoint = getPoint(currentSpacing + 1);
			xtl::xoptional<csm_t> nextHeight = bufferedCsm.extract(nextPoint.x, nextPoint.y, ExtractMethod::bilinear);

			bool boundaryFound = false;
			if (!currentHeight.has_value()) {
				boundaryFound = true;
			}

			while (!boundaryFound) {
				//we've hit the maximum allowed distance
				if (currentSpacing >= nPoints) {
					boundaryFound = true;
				}

				//we've hit the edge of the acquistion
				if (!nextHeight.has_value()) {
					boundaryFound = true;
				}

				//we've hit a local minimum
				if (currentHeight.value() < prevHeight.value() && currentHeight.value() <= nextHeight.value()) {
					boundaryFound = true;
				}

				//we've gone too far below the high point
				if (currentHeight.value() < heightCutoff && prevHeight.value() < heightCutoff && nextHeight.value() < heightCutoff) {
					boundaryFound = true;
				}

				//we've hit a steep drop-off
				csm_t prevDiff = prevHeight.value() - currentHeight.value();
				csm_t nextDiff = currentHeight.value() - nextHeight.value();
				if (prevDiff > 0 && nextDiff > 0) {
					if (nextDiff > prevDiff * _slopeChangeMultiplier) {
						boundaryFound = true;
					}
				}

				if (!boundaryFound) {
					currentSpacing++;
					prevPoint = currentPoint;
					prevHeight = currentHeight;
					currentPoint = nextPoint;
					currentHeight = nextHeight;
					nextPoint = getPoint(currentSpacing + 1);
					nextHeight = bufferedCsm.extract(nextPoint.x, nextPoint.y, ExtractMethod::bilinear);
				}

			}
			vertexDistances.push_back(currentSpacing * pointSpacing);
		}

		switch (_smoothType) {
		case McGaugheySmoothType::fusion:
			_fusionSmooth(vertexDistances);
			break;
		case McGaugheySmoothType::simple:
			_simpleSmooth(vertexDistances);
			break;
		case McGaugheySmoothType::none:
		default:
			break;
		}

		std::vector<CoordXY> ring;
		ring.reserve(_nVertices);
		for (size_t vertex = 0; vertex < _nVertices; ++vertex) {
			ring.push_back(CoordXY{ x + vertexDistances[vertex] * vertexCoss[vertex] ,y + vertexDistances[vertex] * vertexSins[vertex]});
		}

		//get it clockwise
		std::reverse(ring.begin(), ring.end());
		return lapis::Polygon(ring);
    }
	void TaoSegMcGaughey::_fusionSmooth(std::vector<coord_t>& vertexDistances) const
	{
		//this algorithm is copied as closely as possible from the FUSION source code,
		//even where it doesn't really make sense

		//a helper function to deal with the fact that we'll routinely go one past the edge of the vector's size
		auto getVertexDistance = [&](size_t vertex) {
			return vertexDistances[(vertex + vertexDistances.size()) % vertexDistances.size()];
			};

		std::vector<coord_t> smoothedVertexDistances = vertexDistances;

		auto smoothVertex = [&](size_t vertex) {
			smoothedVertexDistances[vertex] = (getVertexDistance(vertex - 1) + getVertexDistance(vertex + 1)) / 2.;
			};

		constexpr coord_t spikeMultiplier = 0.25;

		//technically, the outward and inward spikes could be detected in a single pass by taking the absolute value of the difference
		//but the fact that the smoothing happens in two stages presumably changes the values, so doing this would change the algorithm

		//outward spikes
		for (size_t i = 0; i < _nVertices; ++i) {
			if (
				(getVertexDistance(i) - getVertexDistance(i - 1)) >= (getVertexDistance(i) * spikeMultiplier) ||
				(getVertexDistance(i) - getVertexDistance(i + 1)) >= (getVertexDistance(i) * spikeMultiplier)) {
				smoothVertex(i);
			}
		}
		vertexDistances = smoothedVertexDistances;

		//inward spikes
		for (size_t i = 0; i < _nVertices; ++i) {
			if (
				(getVertexDistance(i - 1) - getVertexDistance(i)) >= (getVertexDistance(i) * spikeMultiplier) ||
				(getVertexDistance(i + 1) - getVertexDistance(i)) >= (getVertexDistance(i) * spikeMultiplier)) {
				smoothVertex(i);
			}
		}
		vertexDistances = smoothedVertexDistances;

		//smooth all outward spikes...again? without a check this time to ensure they're extra-spikey
		for (size_t i = 0; i < _nVertices; ++i) {
			if (
				getVertexDistance(i - 1) < getVertexDistance(i)
				&& getVertexDistance(i + 1) < getVertexDistance(i)
				) {
				smoothVertex(i);
			}
		}
		vertexDistances = smoothedVertexDistances;
	}
	void TaoSegMcGaughey::_simpleSmooth(std::vector<coord_t>& vertexDistances) const
	{
		//assign each vertex distance to a simple three-way average of the previous, current, and next vertex distances

		//a helper function to deal with the fact that we'll routinely go one past the edge of the vector's size
		auto getVertexDistance = [&](size_t vertex) {
			return vertexDistances[vertex % vertexDistances.size()];
			};

		std::vector<coord_t> smoothedVertexDistances = vertexDistances;
		for (size_t i = 0; i < _nVertices; ++i) {
			smoothedVertexDistances[i] = (getVertexDistance(i - 1) + getVertexDistance(i) + getVertexDistance(i + 1)) / 3.;
		}

		vertexDistances = smoothedVertexDistances;
	}
}