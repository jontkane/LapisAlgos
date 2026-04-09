#include"NormalizeByRaster.hpp"

namespace lapis {
    NormalizeByRaster::NormalizeByRaster(std::unique_ptr<LidarStreamer> streamer, const Raster<coord_t>& r)
        : _source(std::move(streamer)), _raster(r) {}
    NormalizeByRaster::NormalizeByRaster(std::unique_ptr<LidarStreamer> streamer, Raster<coord_t>&& r)
        : _source(std::move(streamer)), _raster(std::move(r)) {}
    const std::span<const LasPoint> NormalizeByRaster::getPoints(size_t n)
    {
        _buffer.clear();
        _buffer.reserve(n);
        const std::span<const LasPoint> points = _source->getPoints(n);
        for (const LasPoint& p : points) {
            if (!_raster.contains(p.x, p.y)) {
                continue;
            }
            auto elev = _raster.extract(p.x, p.y, ExtractMethod::bilinear);
            if (!elev.has_value()) {
                continue;
            }
            LasPoint np = p;
            np.z -= elev.value();
            _buffer.push_back(np);
        }
        return _buffer;
    }
    bool NormalizeByRaster::hasMorePoints() const
    {
        return _source->hasMorePoints();
    }
    size_t NormalizeByRaster::nPoints() const
    {
        return _source->nPoints();
    }
    size_t NormalizeByRaster::nPointsRemaining() const
    {
        return _source->nPointsRemaining();
    }
    const CoordRef& NormalizeByRaster::getCoordRef() const
    {
        return _source->getCoordRef();
    }
    const Extent& NormalizeByRaster::getExtent() const
    {
        return _source->getExtent();
    }
    void NormalizeByRaster::reset()
    {
        _source->reset();
    }
    bool NormalizeByRaster::_canModifyInPlace() const
    {
        return true;
    }
    NormalizeByRasterFactory::RasterConfig NormalizeByRasterFactory::addRaster(const std::string& filename)
    {
        return addRaster(std::filesystem::path(filename));
    }
    NormalizeByRasterFactory::RasterConfig NormalizeByRasterFactory::addRaster(const std::filesystem::path& filename)
    {
        Alignment a{ filename.string() };
        return addRaster(filename, a);
    }
    NormalizeByRasterFactory::RasterConfig NormalizeByRasterFactory::addRaster(const std::string& filename, const Alignment& a)
    {
        return addRaster(std::filesystem::path(filename), a);
    }
    NormalizeByRasterFactory::RasterConfig NormalizeByRasterFactory::addRaster(const std::filesystem::path& filename, const Alignment& a)
    {
        RasterOverride ro;
        if (_rasterCrsOverride.has_value()) {
            ro.overrideCrs = _rasterCrsOverride.value();
        }
        if (_rasterZUnitsOverride.has_value()) {
            ro.overrideZUnits = _rasterZUnitsOverride.value();
        }
        ro.variant = RasterMetadata{ filename, a };
        _rasters.push_back(ro);
        return RasterConfig(*this, _rasters.size() - 1);
    }
    NormalizeByRasterFactory::RasterConfig NormalizeByRasterFactory::addRaster(const Raster<coord_t>& r)
    {
        RasterOverride ro;
        if (_rasterCrsOverride.has_value()) {
            ro.overrideCrs = _rasterCrsOverride.value();
        }
        if (_rasterZUnitsOverride.has_value()) {
            ro.overrideZUnits = _rasterZUnitsOverride.value();
        }
        ro.variant = r;
        _rasters.push_back(ro);
        return RasterConfig(*this, _rasters.size() - 1);
    }
    NormalizeByRasterFactory::RasterConfig NormalizeByRasterFactory::addRaster(Raster<coord_t>&& r)
    {
        RasterOverride ro;
        if (_rasterCrsOverride.has_value()) {
            ro.overrideCrs = _rasterCrsOverride.value();
        }
        if (_rasterZUnitsOverride.has_value()) {
            ro.overrideZUnits = _rasterZUnitsOverride.value();
        }
        ro.variant = std::move(r);
        _rasters.push_back(ro);
        return RasterConfig(*this, _rasters.size() - 1);
    }
    void NormalizeByRasterFactory::setCrsOverride(const CoordRef& crs)
    {
        _rasterCrsOverride = crs;
    }
    void NormalizeByRasterFactory::clearCrsOverride()
    {
        _rasterCrsOverride.reset();
    }
    void NormalizeByRasterFactory::setZUnitsOverride(const LinearUnit& zUnits)
    {
        _rasterZUnitsOverride = zUnits;
    }
    void NormalizeByRasterFactory::clearZUnitsOverride()
    {
        _rasterZUnitsOverride.reset();
    }
    std::unique_ptr<LidarStreamer> NormalizeByRasterFactory::create(std::unique_ptr<LidarStreamer> streamer) const
    {

        Extent e = streamer->getExtent();
        const CoordRef& crs = streamer->getCoordRef();

        coord_t fiveMeters = linearUnitPresets::meter.convertOneFromThis(5, crs.getXYLinearUnits());

        Extent buffer = bufferExtent(e, fiveMeters);

        std::vector<Raster<coord_t>> overlappingDems;

        for (const RasterOverride& ro : _rasters) {
            Extent projE;
            Raster<coord_t> dem;

            const RasterVariant& rv = ro.variant;

            if (std::holds_alternative<RasterMetadata>(rv)) {
                const RasterMetadata& md = std::get<RasterMetadata>(rv);
                projE = QuadExtent(md.alignment, crs).outerExtent();
                if (!projE.overlaps(buffer)) {
                    continue;
                }
                try {
                    dem = Raster<coord_t>(md.filename.string(), buffer, SnapType::out);
                    if (ro.overrideCrs.has_value()) {
                        dem.defineCRS(ro.overrideCrs.value());
                    }
                    if (ro.overrideZUnits.has_value()) {
                        dem.setZUnits(ro.overrideZUnits.value());
                    }
                }
                catch (...) {
                    continue;
                }
            }
            else {
                dem = std::get<Raster<coord_t>>(rv);
                if (ro.overrideCrs.has_value()) {
                    dem.defineCRS(ro.overrideCrs.value());
                }
                if (ro.overrideZUnits.has_value()) {
                    dem.setZUnits(ro.overrideZUnits.value());
                }
                projE = QuadExtent(dem, crs).outerExtent();
                if (!projE.overlaps(buffer)) {
                    continue;
                }
            }
            overlappingDems.push_back(std::move(dem));
        }

        if (!overlappingDems.size()) {
            throw std::runtime_error("No DEM rasters overlap the chosen extent in NormalizeByRasterFactory::create");
        }

        auto resInCrsUnits = [](const Alignment& a, const CoordRef& crs) {
            Alignment transformed = transformAlignment(a, crs);
            return std::min(transformed.xres(), transformed.yres());
            };
        std::sort(overlappingDems.begin(), overlappingDems.end(), [&](const Raster<coord_t>& a, const Raster<coord_t>& b) {
            return resInCrsUnits(a, crs) < resInCrsUnits(b, crs);
            });
        Alignment finalAlign = transformAlignment(overlappingDems.front(), crs);
        finalAlign = extendAlignment(finalAlign, buffer, SnapType::out);
        finalAlign = cropAlignment(finalAlign, buffer, SnapType::out);
        Raster<coord_t> finalDem{ finalAlign };

        for (Raster<coord_t>& dem : overlappingDems) {
            if (!dem.consistentAlignment(finalDem)) {
                dem = resampleRaster(dem, finalDem, ExtractMethod::bilinear);
            }
            if (!dem.crs().isConsistentZUnits(crs)) {
                LinearUnitConverter converter{ dem.crs().getZUnits(), crs.getZUnits() };
                converter.convertManyInPlace(&dem[0].value(), dem.ncell(), sizeof(coord_t));
            }
            finalDem.overlay(dem, [](coord_t a, coord_t b) { return a; });
        }

        return std::unique_ptr<LidarStreamer>(new NormalizeByRaster(std::move(streamer), std::move(finalDem)));
    }

    NormalizeByRasterFactory::RasterConfig::RasterConfig(NormalizeByRasterFactory& parent, size_t index)
        : _parent(parent), _index(index)
    {}
    NormalizeByRasterFactory::RasterConfig& NormalizeByRasterFactory::RasterConfig::setCrs(const CoordRef& crs)
    {
        _parent._rasters[_index].overrideCrs = crs;
        return *this;
    }
    NormalizeByRasterFactory::RasterConfig& NormalizeByRasterFactory::RasterConfig::setZUnits(const LinearUnit& zUnits)
    {
        _parent._rasters[_index].overrideZUnits = zUnits;
        return *this;
    }
}