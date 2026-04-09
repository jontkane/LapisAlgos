#pragma once
#ifndef LIDARSTREAMERBUILDER_HPP
#define LIDARSTREAMERBUILDER_HPP

#include"algos_pch.hpp"
#include"LidarStreamer.hpp"
#include"NormalizeByRaster.hpp"

/*
* Example of use:
* 
* LidarStreamerBuilder builder;
* // add the ground models, if you're normalizing by raster. adding a dem will return a config object where you can override the header information if you know the units of the raster
* builder.addDem("dem.tif").setZUnits(linearUnitPresets::meter);
* 
* // set various configurations.
* builder.setLasZUnitsOverride(linearUnitPresets::meter).setExtent(e).addDefaultFilters();
* 
* // set up the pipeline that the points are sent through. They are applied in the order you specify
* builder.normalizeByDem().apply<LidarStreamerMinMaxFilter>(0,100).apply<LidarStreamerReprojector>(targetCrs);
* 
* // request a LasSpecifier. This is a thread-safe way to specify the las files used as input. You may request multiple of these from the same builder.
* auto lasSpec = builder.createLasSpecifier().addLas("file1.laz").addLas("file2.laz");
* 
* //get your final streamer
* 
* auto streamer1 = lasSpec.buildInMemory(); //if you want to load all the points into memory, which is faster if you intend to iterate through the points multiple times
* auto streamer2 = lasSpec.buildHardDrive(); //if you want to keep the points on the hard drive, which uses less memory
*/

namespace lapis {
    class LidarStreamerBuilder {
        class LasSpecifier;
    public:
        LidarStreamerBuilder() = default;

        LasSpecifier createLasSpecifier();

        LidarStreamerBuilder& setLasCrsOverride(const CoordRef& crs);
        LidarStreamerBuilder& clearLasCrsOverride();
        LidarStreamerBuilder& setLasZUnitsOverride(const LinearUnit& zUnits);
        LidarStreamerBuilder& clearLasZUnitsOverride();

        using RasterConfig = decltype(std::declval<NormalizeByRasterFactory>().addRaster(std::declval<std::string>()));
        RasterConfig addDem(const std::string& filename);
        RasterConfig addDem(const std::filesystem::path& filepath);
        RasterConfig addDem(const std::string& filename, const Alignment& a);
        RasterConfig addDem(const std::filesystem::path& filepath, const Alignment& a);
        RasterConfig addDem(const Raster<coord_t>& raster);
        RasterConfig addDem(Raster<coord_t>&& raster);

        LidarStreamerBuilder& setDemCrsOverride(const CoordRef& crs);
        LidarStreamerBuilder& clearDemCrsOverride();
        LidarStreamerBuilder& setDemZUnitsOverride(const LinearUnit& zUnits);
        LidarStreamerBuilder& clearDemZUnitsOverride();
        
        LidarStreamerBuilder& setExtent(const Extent& extent);

        LidarStreamerBuilder& addFilter(std::shared_ptr<LasFilter> filter);
        LidarStreamerBuilder& addDefaultFilters();

        //add a decorator, such as reprojection or normalization. Decorators are applied in the order they are added.
        template<class Decorator, class... Args>
        LidarStreamerBuilder& apply(Args&&... args);

        //similar to apply, but for NormalizeByRaster. It will apply a normalization based on a mosaic of the dem file specified.
        LidarStreamerBuilder& normalizeByDem();

        void reset();

    private:
        
        std::optional<NormalizeByRasterFactory> _normalizeFactory;
        std::optional<Extent> _extent;
        std::vector<std::shared_ptr<LasFilter>> _filters;

        std::function<std::unique_ptr<LidarStreamer>(std::unique_ptr<LidarStreamer>)> _decoratorChain;

        std::optional<CoordRef> _lasCrsOverride;
        std::optional<LinearUnit> _lasZUnitsOverride;

        class LasSpecifier {
            class LasConfig;
        public:
            LasSpecifier(const LidarStreamerBuilder& parent);

            LasConfig addLas(const std::string& filename);
            LasConfig addLas(const std::filesystem::path& filepath);

            std::unique_ptr<LidarStreamer> buildInMemory() const;
            std::unique_ptr<LidarStreamer> buildHardDrive() const;


        private:
            const LidarStreamerBuilder* _parent;
            struct LasOverride {
                std::filesystem::path filename;
                std::optional<CoordRef> overrideCrs;
                std::optional<LinearUnit> overrideZUnits;
            };
            std::vector<LasOverride> _lasFiles;

            std::unique_ptr<LidarStreamer> _buildBaseStreamer() const;

            class LasConfig {
            public:
                LasConfig(LasSpecifier& parent, size_t index);
                LasConfig& setCrs(const CoordRef& crs);
                LasConfig& setZUnits(const LinearUnit& zUnits);
            private:
                LasSpecifier& _parent;
                size_t _index;
            };
        };

        
    };

    template<class Decorator, class... Args>
    LidarStreamerBuilder& LidarStreamerBuilder::apply(Args&&... args) {
        auto oldChain = std::move(_decoratorChain);

        _decoratorChain = [oldChain = std::move(oldChain),
            ...capturedArgs = std::forward<Args>(args)]
            (std::unique_ptr<LidarStreamer> base) mutable {
            auto decorated = oldChain ? oldChain(std::move(base)) : std::move(base);
            return std::make_unique<Decorator>(std::move(decorated), capturedArgs...);
            };

        return *this;
    }
}


#endif