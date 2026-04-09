#include"CsmMakerBuilder.hpp"
#include"CsmMaxHeight.hpp"

namespace lapis {
    CsmMakerBuilder CsmMakerBuilder::maxHeight()
    {
        CsmMakerBuilder out;
        out._buildFunc = [](const Alignment& a) {
            return std::make_unique<CsmMaxHeight>(a);
            };
        return out;
    }
    CsmMakerBuilder::CsmMosaicBuilder CsmMakerBuilder::fromMosaic(const CsmMaker::CsmMergeFunc& mergeFunc)
    {
        return CsmMosaicBuilder(mergeFunc);
    }
    std::unique_ptr<CsmMaker> CsmMakerBuilder::build(const Alignment& a) const
    {
        return _buildFunc(a);
    }
    CsmMaker::CsmMergeFunc CsmMakerBuilder::mergeFunction() const
    {
        //may need to revisit this if any future CsmMakers have expensive constructors
        return build(Alignment())->getMergeFunction();
    }
    CsmMakerBuilder::CsmMosaicBuilder::CsmMosaicBuilder(const CsmMaker::CsmMergeFunc& mergeFunc)
        : _mergeFunc(mergeFunc)
    {
    }
    CsmMakerBuilder::CsmMosaicBuilder& CsmMakerBuilder::CsmMosaicBuilder::addRaster(const std::string& filename)
    {
        _mosaicer.addRaster(filename);
        return *this;
    }
    CsmMakerBuilder::CsmMosaicBuilder& CsmMakerBuilder::CsmMosaicBuilder::addRaster(const std::filesystem::path& filename)
    {
        _mosaicer.addRaster(filename);
        return *this;
    }
    CsmMakerBuilder::CsmMosaicBuilder& CsmMakerBuilder::CsmMosaicBuilder::addRaster(const std::string& filename, const Alignment& a)
    {
        _mosaicer.addRaster(filename, a);
        return *this;
    }
    CsmMakerBuilder::CsmMosaicBuilder& CsmMakerBuilder::CsmMosaicBuilder::addRaster(const std::filesystem::path& filename, const Alignment& a)
    {
        _mosaicer.addRaster(filename, a);
        return *this;
    }
    CsmMakerBuilder::CsmMosaicBuilder& CsmMakerBuilder::CsmMosaicBuilder::addRaster(const Raster<csm_t>& r)
    {
        _mosaicer.addRaster(r);
        return *this;
    }
    CsmMakerBuilder::CsmMosaicBuilder& CsmMakerBuilder::CsmMosaicBuilder::addRaster(Raster<csm_t>&& r)
    {
        _mosaicer.addRaster(std::move(r));
        return *this;
    }
    CsmMakerBuilder::CsmMosaicBuilder& CsmMakerBuilder::CsmMosaicBuilder::assumeAllRastersAlign()
    {
        _assumeAllRastersAlign = true;
        return *this;
    }
    std::unique_ptr<CsmMaker> CsmMakerBuilder::CsmMosaicBuilder::build(const Alignment& a) const
    {
        std::unique_ptr<CsmMaker> out;
        if (_assumeAllRastersAlign) {
            out = _mosaicer.createAssumeAllRastersAlign(a, _mergeFunc);
        }
        else {
            out = _mosaicer.create(a, _mergeFunc);
        }
        if (_applyDecorators) {
            out = _applyDecorators(std::move(out));
        }
        return out;
    }
}