#include"TaoBuilder.hpp"
#include"AllTaoAlgos.hpp"

namespace lapis {
    TaoBuilder TaoBuilder::highPoints(csm_t minHtCsmZUnits, coord_t minDistCsmXYUnits)
    {
        TaoBuilder builder;
        builder._idAlgo = std::make_unique<TaoIdHighPoints>(minHtCsmZUnits, minDistCsmXYUnits);
        return builder;
    }
    void TaoBuilder::addWatershed(csm_t minHt, csm_t maxHt)
    {
       addWatershed(TaoSegWatershed::name(), minHt, maxHt);
    }
    void TaoBuilder::addWatershedDefaults()
    {
        addWatershedDefaults(TaoSegWatershed::name());
    }
    void TaoBuilder::addWatershed(const std::string& name, csm_t minHt, csm_t maxHt)
    {
        addSegmenter<TaoSegWatershed>(name, minHt, maxHt);
    }
    void TaoBuilder::addWatershedDefaults(const std::string& name)
    {
        addWatershed(name, _minHtForIdAlgo, 500);
    }
    void TaoBuilder::addMcGaughey(int nVertices, csm_t slopeChangeMultiplier, csm_t heightCutoffMultiplier, coord_t maxDistMultiplier, McGaugheySmoothType smoothType)
    {
        addMcGaughey(TaoSegMcGaughey::name(), nVertices, slopeChangeMultiplier, heightCutoffMultiplier, maxDistMultiplier, smoothType);
    }
    void TaoBuilder::addMcGaugheyFusionDefaults()
    {
        addMcGaugheyFusionDefaults(TaoSegMcGaughey::name());
    }
    void TaoBuilder::addMcGaughey(const std::string& name, int nVertices, csm_t slopeChangeMultiplier, csm_t heightCutoffMultiplier, coord_t maxDistMultiplier, McGaugheySmoothType smoothType)
    {
        addSegmenter<TaoSegMcGaughey>(name, nVertices, slopeChangeMultiplier, heightCutoffMultiplier, maxDistMultiplier, smoothType);
    }
    void TaoBuilder::addMcGaugheyFusionDefaults(const std::string& name)
    {
        addMcGaughey(name, 16, 4, 2.0 / 3, 3.0 / 4, McGaugheySmoothType::fusion);
    }
    std::vector<IDedTao> TaoBuilder::_processIdAlgo(TaoResults& results, const Raster<csm_t>& bufferedCsm, const Extent& unbufferedExtent, TaoIdGenerator idGen) const
    {
        std::vector<IDedTao> idResults = _idAlgo->process(bufferedCsm, idGen);

        VectorDataset<Point>& points = results.taos;
        points.defineCrs(bufferedCsm.crs());
        points.addNumericField<taoid_t>("ID");
        points.addNumericField<coord_t>("X");
        points.addNumericField<coord_t>("Y");
        points.addNumericField<csm_t>("Height");

        for (const IDedTao& idResult : idResults) {
            coord_t x = bufferedCsm.xFromCellUnsafe(idResult.location);
            coord_t y = bufferedCsm.yFromCellUnsafe(idResult.location);
            auto height = bufferedCsm.atCellUnsafe(idResult.location);
            if (!height.has_value()) {
                continue; //this shouldn't happen
            }

            //using half-open guarantees that extens that at most share an edge will have disjoint results
            if (!unbufferedExtent.containsHalfOpen(x, y)) {
                continue;
            }

            Point p(x, y, bufferedCsm.crs());
            points.addGeometryUnsafe(p);
            points.back().setNumericField<taoid_t>("ID", idResult.id);
            points.back().setNumericField("X", x);
            points.back().setNumericField("Y", y);
            points.back().setNumericField<csm_t>("Height", height.value());
        }

        return idResults;
    }
    void TaoBuilder::_processOneSegmenter(TaoResults& results, const std::string& name, const Raster<csm_t>& bufferedCsm, const Extent& unbufferedExtent, const std::vector<IDedTao>& taos) const
    {
        if (!_segmenters.contains(name)) {
            throw std::runtime_error("TaoBuilder: No segmenter with name " + name);
       }
        TaoResults::SegmentResults& out = results.segmenterResults[name]; //default construct intended

        std::unordered_map<taoid_t, size_t> idToIndex;
        for (size_t i = 0; i < results.taos.nFeature(); ++i) {
            idToIndex.emplace(results.taos.getNumericField<taoid_t>(i, "ID"), i);
        }

        auto segmenterResults = _segmenters.at(name)->process(bufferedCsm, unbufferedExtent, taos);
        if (segmenterResults.segmentRaster.has_value()) {
            out.segmentRaster = std::move(segmenterResults.segmentRaster);

            out.taoHeightRaster = Raster<csm_t>((Alignment)*out.segmentRaster);
            for (cell_t cell : CellIterator(*out.taoHeightRaster)) {
                auto idV = out.segmentRaster->atCellUnsafe(cell);
                auto taoHeightV = out.taoHeightRaster->atCellUnsafe(cell);
                if (!idV.has_value()) {
                    taoHeightV.has_value() = false;
                    continue;
                }
                csm_t height = results.taos.getNumericField<csm_t>(idToIndex.at(idV.value()), "Height");
                taoHeightV.value() = height;
                taoHeightV.has_value() = true;
            }
        }
        if (segmenterResults.segmentPolygons.has_value()) {
            out.segmentPolygons = std::move(segmenterResults.segmentPolygons);
            //right now, the attribute table only has ID. We need X, Y, Height, and Area
            VectorDataset<MultiPolygon>& polys = out.segmentPolygons.value();
            polys.addNumericField<coord_t>("X");
            polys.addNumericField<coord_t>("Y");
            polys.addNumericField<csm_t>("Height");
            polys.addNumericField<coord_t>("Area");

            for (auto feature : polys) {
                taoid_t id = feature.getNumericField<taoid_t>("ID");
                if (!idToIndex.contains(id)) {
                    throw std::runtime_error("TaoBuilder: Segmenter returned a polygon with an ID that wasn't in the original TAO results");
                }
                size_t index = idToIndex.at(id);
                coord_t x = results.taos.getNumericField<coord_t>(index, "X");
                coord_t y = results.taos.getNumericField<coord_t>(index, "Y");
                csm_t height = results.taos.getNumericField<csm_t>(index, "Height");

                feature.setNumericField<coord_t>("X", x);
                feature.setNumericField<coord_t>("Y", y);
                feature.setNumericField<csm_t>("Height", height);
                feature.setNumericField<coord_t>("Area", feature.getGeometry().area());
            }
        }
    }
    TaoBuilder::TaoResults TaoBuilder::process(const Raster<csm_t>& bufferedCsm, const Extent& unbufferedExtent, TaoIdGenerator idGen) const
    {

        if (!_idAlgo) {
            throw std::runtime_error("TaoBuilder must have an ID algorithm before processing");
        }
        if (!bufferedCsm.crs().isConsistent(unbufferedExtent.crs())) {
            throw std::runtime_error("TaoBuilder: CRS of bufferedCsm and unbufferedExtent must be consistent");
        }
        if (bufferedCsm.xmin() > unbufferedExtent.xmin() || bufferedCsm.xmax() < unbufferedExtent.xmax() ||
            bufferedCsm.ymin() > unbufferedExtent.ymin() || bufferedCsm.ymax() < unbufferedExtent.ymax()) {
            throw std::runtime_error("TaoBuilder: bufferedCsm must cover the entire area of unbufferedExtent");
        }

        TaoResults results;

        std::vector<IDedTao> idResults = _processIdAlgo(results, bufferedCsm, unbufferedExtent, idGen);
        
        for (const auto& [name, segmenter] : _segmenters) {
            _processOneSegmenter(results, name, bufferedCsm, unbufferedExtent, idResults);
        }

        return results;
    }
}