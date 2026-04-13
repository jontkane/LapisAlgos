#pragma once
#ifndef TAOBUILDER_HPP
#define TAOBUILDER_HPP

#include"algos_pch.hpp"
#include"TaoId.hpp"
#include"TaoSeg.hpp"
#include"TaoSegMcGaughey.hpp"

namespace lapis {

    /*
    * Example usage:
    * 
    * TaoBuilder builder = TaoBuilder::highPoints(2, 0);
    * builder.addMcGaugheyFusionDefaults();
    * builder.addMcGaughey("McGaughey2", 8, 4, 2./3, 3./4, McGaugheySmoothType::simple);
    * builder.addWatershedDefaults();
    * 
    * //the buffered csm helps to avoid edge effects. If that's not a concern in your application, simply pass the raster's extent in for no buffer
    * //TaoIdGenerator{1,1} means the first TAO will have ID 1, and the IDs will increment by 1 for each subsequent TAO
    * //if you have multiple tiles, you can use this to ensure globally unique IDs
    * auto results = builder.process(bufferedCsm, unbufferedExtent, TaoIdGenerator{1,1});
    * 
    * VectorDataset<Point>& highPoints = results.taos;
    * 
    * auto& watershedResults = results.getSegmentsIfUnique<WatershedSeg>(); //works because there's only one watershed segmenter.
    * 
    * //need to use the name because there are multiple McGaughey segmenters. The default name is stored in the static name() function of the segmenter class
    * auto& mcGaugheyResults = results.segmenterResults.at(TaoSegMcGaughey::name());
    * 
    * //the segment results are optional because not all algorithms have both vectors and rasters
    * Raster<taoid_t>& watershedSegmentRaster = watershedResults.segmentRaster.value();
    * Raster<csm_t>& watershedHeightRaster = watershedResults.taoHeightRaster.value();
    * VectorDataset<MultiPolygon>& watershedSegmentPolygons = watershedResults.segmentPolygons.value();
    */

    class TaoBuilder {
    public:
        static TaoBuilder highPoints(csm_t minHtCsmZUnits, coord_t minDistCsmXYUnits);
        
        //uses a default name based on the class of the segmenter. Calling this function multiple times with the same class will throw
        template<class TAOSEG, class... Args>
        void addSegmenter(Args&&... args);

        //custom name for when you want multiple segmenters of the same class
        template<class TAOSEG, class... Args>
        void addSegmenter(const std::string& name, Args&&... args);

        //minHt is the minimum height that will ever be part of a segment; values below that are treated as NoData
        //maxHt controls memory allocation. Values above maxHt will be treated as equal to maxHt
        //uses default name; adding watershed multiple times will throw
        void addWatershed(csm_t minHt, csm_t maxHt);
        //minHt will be the same value you pass to the constructor
        //maxHt will be 500, which should be sufficient for both feet and meters. It may allocate more memory than necessary.
        //uses default name; adding watershed multiple times will throw
        void addWatershedDefaults();

        //minHt is the minimum height that will ever be part of a segment; values below that are treated as NoData
        //maxHt controls memory allocation. Values above maxHt will be treated as equal to maxHt
        //custom name for when you want multiple watershed segmenters
        void addWatershed(const std::string& name, csm_t minHt, csm_t maxHt);
        //minHt will be the same value you pass to the constructor
        //maxHt will be 500, which should be sufficient for both feet and meters. It may allocate more memory than necessary.
        //custom name for when you want multiple watershed segmenters
        void addWatershedDefaults(const std::string& name);


        //uses default name; adding McGaughey multiple times will throw
        void addMcGaughey(int nVertices,
            csm_t slopeChangeMultiplier,
            csm_t heightCutoffMultiplier,
            coord_t maxDistMultiplier,
            McGaugheySmoothType smoothType);
        //The same parameters used in FUSION
        //nVertices = 16, slopeChangeMultiplier = 4, heightCutoffMultiplier = 2/3, maxDistMultiplier = 3/4, smoothType = fusion
        //uses default name; adding McGaughey multiple times will throw
        void addMcGaugheyFusionDefaults();

        //custom name for when you want multiple McGaughey segmenters
        void addMcGaughey(const std::string& name,
            int nVertices,
            csm_t slopeChangeMultiplier,
            csm_t heightCutoffMultiplier,
            coord_t maxDistMultiplier,
            McGaugheySmoothType smoothType);
        //The same parameters used in FUSION
        //nVertices = 16, slopeChangeMultiplier = 4, heightCutoffMultiplier = 2/3, maxDistMultiplier = 3/4, smoothType = fusion
        //custom name for when you want multiple McGaughey segmenters
        void addMcGaugheyFusionDefaults(const std::string& name);


        struct TaoResults {
            struct SegmentResults {
                std::optional<VectorDataset<MultiPolygon>> segmentPolygons;
                std::optional<Raster<taoid_t>> segmentRaster;
                std::optional<Raster<csm_t>> taoHeightRaster;
            };

            VectorDataset<Point> taos;
            std::unordered_map<std::string, SegmentResults> segmenterResults;

            template<class SEGALGO>
            SegmentResults& getSegmentsIfUnique();
        };
        TaoResults process(const Raster<csm_t>& bufferedCsm, const Extent& unbufferedExtent, TaoIdGenerator idGen) const;

    private:
        TaoBuilder() = default;

        std::unique_ptr<TaoIdAlgo> _idAlgo;
        std::unordered_map<std::string, std::unique_ptr<TaoSegAlgo>> _segmenters;
        csm_t _minHtForIdAlgo = 2;

        std::vector<IDedTao> _processIdAlgo(TaoResults& results, const Raster<csm_t>& bufferedCsm, const Extent& unbufferedExtent, TaoIdGenerator idGen) const;
        void _processOneSegmenter(TaoResults& results, const std::string& name,
            const Raster<csm_t>& bufferedCsm, const Extent& unbufferedExtent, const std::vector<IDedTao>& taos) const;
    };
    template<class TAOSEG, class ...Args>
    inline void TaoBuilder::addSegmenter(Args&& ...args)
    {
        std::string name = TAOSEG::name();
        if (_segmenters.contains(name)) {
            throw std::runtime_error("TaoBuilder already has a segmenter with the name " + name);
        }
        _segmenters[name] = std::make_unique<TAOSEG>(std::forward<Args>(args)...);
    }
    template<class TAOSEG, class ...Args>
    inline void TaoBuilder::addSegmenter(const std::string& name, Args && ...args)
    {
        if (_segmenters.contains(name)) {
            throw std::runtime_error("TaoBuilder already has a segmenter with the name " + name);
        }
        _segmenters[name] = std::make_unique<TAOSEG>(std::forward<Args>(args)...);
    }
    template<class SEGALGO>
    inline TaoBuilder::TaoResults::SegmentResults& TaoBuilder::TaoResults::getSegmentsIfUnique()
    {
        return segmenterResults.at(SEGALGO::name());
    }
}


#endif