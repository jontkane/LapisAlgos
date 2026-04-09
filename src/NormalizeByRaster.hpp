#pragma once
#ifndef NORMALIZEBYRASTER_HPP
#define NORMALIZEBYRASTER_HPP

#include"algos_pch.hpp"
#include"LidarStreamer.hpp"

namespace lapis {
    class NormalizeByRaster : public LidarStreamer {
    public:
        NormalizeByRaster(std::unique_ptr<LidarStreamer> streamer, const Raster<coord_t>& r);
        NormalizeByRaster(std::unique_ptr<LidarStreamer> streamer, Raster<coord_t>&& r);
        const std::span<const LasPoint> getPoints(size_t n) override;
        bool hasMorePoints() const override;
        size_t nPoints() const override;
        size_t nPointsRemaining() const override;
        const CoordRef& getCoordRef() const override;
        const Extent& getExtent() const override;
        void reset() override;
    protected:
        bool _canModifyInPlace() const override;
    private:
        std::unique_ptr<LidarStreamer> _source;
        Raster<coord_t> _raster;
        std::vector<LasPoint> _buffer;
    };

    class NormalizeByRasterFactory {
        class RasterConfig;
    public:
        NormalizeByRasterFactory() = default;

        RasterConfig addRaster(const std::string& filename);
        RasterConfig addRaster(const std::filesystem::path& filename);
        RasterConfig addRaster(const std::string& filename, const Alignment& a);
        RasterConfig addRaster(const std::filesystem::path& filename, const Alignment& a);
        RasterConfig addRaster(const Raster<coord_t>& r);
        RasterConfig addRaster(Raster<coord_t>&& r);

        void setCrsOverride(const CoordRef& crs);
        void clearCrsOverride();
        void setZUnitsOverride(const LinearUnit& zUnits);
        void clearZUnitsOverride();

        std::unique_ptr<LidarStreamer> create(std::unique_ptr<LidarStreamer> streamer) const;

    private:

        struct RasterMetadata {
            std::filesystem::path filename;
            Alignment alignment;
        };
        using RasterVariant = std::variant<RasterMetadata, Raster<coord_t>>;
        struct RasterOverride {
            RasterVariant variant;
            std::optional<CoordRef> overrideCrs;
            std::optional<LinearUnit> overrideZUnits;
        };
        mutable std::vector<RasterOverride> _rasters;
        std::optional<CoordRef> _rasterCrsOverride;
        std::optional<LinearUnit> _rasterZUnitsOverride;

        class RasterConfig {
        public:
            RasterConfig(NormalizeByRasterFactory& parent, size_t index);
            RasterConfig& setCrs(const CoordRef& crs);
            RasterConfig& setZUnits(const LinearUnit& zUnits);
        private:
            NormalizeByRasterFactory& _parent;
            size_t _index;
        };
    };
};

#endif