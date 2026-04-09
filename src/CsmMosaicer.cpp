#include"CsmMosaicer.hpp"

namespace lapis {
    void CsmMosaicerFactory::addRaster(const std::string& filename)
    {
        addRaster(std::filesystem::path(filename));
    }
    void CsmMosaicerFactory::addRaster(const std::filesystem::path& filename)
    {
        Alignment a{ filename.string() };
        _rasters.emplace_back(RasterMetadata{ filename, a });
    }
    void CsmMosaicerFactory::addRaster(const std::string& filename, const Alignment& a)
    {
        _rasters.emplace_back(RasterMetadata{ std::filesystem::path(filename), a });
    }
    void CsmMosaicerFactory::addRaster(const std::filesystem::path& filename, const Alignment& a)
    {
        _rasters.emplace_back(RasterMetadata{ filename, a });
    }
    void CsmMosaicerFactory::addRaster(const Raster<csm_t>& r)
    {
        _rasters.emplace_back(r);
    }

    void CsmMosaicerFactory::addRaster(Raster<csm_t>&& r)
    {
        _rasters.emplace_back(std::move(r));
    }

    std::unique_ptr<CsmMaker> CsmMosaicerFactory::create(const Alignment& a, const CsmMaker::CsmMergeFunc& mergeFunc) const
    {
        Raster<csm_t> outRaster{ a };

        for (const auto& rasterVariant : _rasters) {
            Alignment otherAlign;
            Raster<csm_t> otherRaster;
            Extent projE;
            if (std::holds_alternative<RasterMetadata>(rasterVariant)) {
                const auto& metadata = std::get<RasterMetadata>(rasterVariant);
                otherAlign = metadata.alignment;
            }
            else {
                const auto& r = std::get<Raster<csm_t>>(rasterVariant);
                otherAlign = r;
            }

            projE = a;
            if (!a.crs().isConsistentHoriz((otherAlign.crs()))) {
                projE = QuadExtent(a, otherAlign.crs()).outerExtent();
                otherAlign = transformAlignment(otherAlign, a.crs());
            }
            if (!a.overlaps(otherAlign)) {
                continue;
            }

            if (std::holds_alternative<RasterMetadata>(rasterVariant)) {
                const auto& metadata = std::get<RasterMetadata>(rasterVariant);
                otherRaster = Raster<csm_t>{ metadata.filename.string(), projE, SnapType::out };
            }
            else {
                const auto& r = std::get<Raster<csm_t>>(rasterVariant);
                otherRaster = cropRaster(r, projE, SnapType::out);
            }

            //this slightly weird structure is to avoid double calling consistentAlignment(), both here and inside overlay()
            try {
                outRaster.overlay(otherRaster, mergeFunc);
            }
            catch (AlignmentMismatchException) {
                otherRaster = resampleRaster(otherRaster, a, ExtractMethod::bilinear);
                outRaster.overlay(otherRaster, mergeFunc);
            }
        }
        return std::make_unique<CsmPreMade>(std::move(outRaster), mergeFunc);
    }

    std::unique_ptr<CsmMaker> CsmMosaicerFactory::createAssumeAllRastersAlign(const Alignment& a, const CsmMaker::CsmMergeFunc& mergeFunc) const
    {
        //the philosophy here is to mostly match the other create() function
        //the major difference is that, because the caller guarantees the rasters align, I will set the CoordRef of the rasters to match the alignment
        //this will skip the potentially expensive PROJ call to check for same crs
        //I will not ever resample; if the rasters don't align, that's an error and I will throw

        Raster<csm_t> outRaster{ a };
        Alignment noCrsAlign{ a };
        noCrsAlign.defineCRS(CoordRef());

        for (const auto& rasterVariant : _rasters) {
            Alignment otherAlign;
            Raster<csm_t> otherRaster;
            if (std::holds_alternative<RasterMetadata>(rasterVariant)) {
                const auto& metadata = std::get<RasterMetadata>(rasterVariant);
                otherAlign = metadata.alignment;
            }
            else {
                const auto& r = std::get<Raster<csm_t>>(rasterVariant);
                otherAlign = r;
            }

            otherAlign.defineCRS(a.crs());
            if (!a.overlaps(otherAlign)) {
                continue;
            }
            if (!a.consistentAlignment(otherAlign)) {
                throw AlignmentMismatchException("Alignment mismatch in createAllRastersAlign");
            }


            if (std::holds_alternative<RasterMetadata>(rasterVariant)) {
                const auto& metadata = std::get<RasterMetadata>(rasterVariant);
                otherRaster = Raster<csm_t>{ metadata.filename.string(), a, SnapType::out };
            } else {
                const auto& r = std::get<Raster<csm_t>>(rasterVariant);
                otherRaster = cropRaster(r, noCrsAlign, SnapType::out);
            }


            otherRaster.defineCRS(a.crs());
            outRaster.overlay(otherRaster, mergeFunc);
        }

        return std::make_unique<CsmPreMade>(std::move(outRaster), mergeFunc);
    }

}