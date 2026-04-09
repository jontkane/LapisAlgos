#pragma once
#ifndef CSMMOSAICER_HPP
#define CSMMOSAICER_HPP

#include"algos_pch.hpp"
#include"CsmMaker.hpp"
#include"CsmPreMade.hpp"

namespace lapis {
    class CsmMosaicerFactory {
    public:
        CsmMosaicerFactory() = default;

        void addRaster(const std::string& filename);
        void addRaster(const std::filesystem::path& filename);

        void addRaster(const std::string& filename, const Alignment& a);
        void addRaster(const std::filesystem::path& filename, const Alignment& a);

        void addRaster(const Raster<csm_t>& r);
        void addRaster(Raster<csm_t>&& r);

        std::unique_ptr<CsmMaker> create(const Alignment& a, const CsmMaker::CsmMergeFunc& mergeFunc) const;
        std::unique_ptr<CsmMaker> createAssumeAllRastersAlign(const Alignment& a, const CsmMaker::CsmMergeFunc& mergeFunc) const;

    private:
        struct RasterMetadata {
            std::filesystem::path filename;
            Alignment alignment;
        };
        using RasterVariant = std::variant<RasterMetadata, Raster<csm_t>>;
        mutable std::vector<RasterVariant> _rasters;
    };
}

#endif