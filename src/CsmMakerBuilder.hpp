#pragma once
#ifndef CSMMAKERBUILDER_HPP
#define CSMMAKERBUILDER_HPP

#include"algos_pch.hpp"
#include"CsmMaker.hpp"
#include"CsmMosaicer.hpp"

/*
* Example usage:
* 
* //initial construction
* CsmMakerBuilder builder = CsmMakerBuilder::maxHeight();
* 
* //add decorators
* builder.apply<CsmSmoother>(3).apply<CsmFiller>(6, 5.);
* 
* //build the maker
* std::unique_ptr<CsmMaker> maker = builder.build(align);
* 
* //make the csm
* while (lidarStreamer.hasMorePoints()) {
*   maker->addPoints(lidarStreamer.getPoints(10000));
* }
* Raster<csm_t> csm = maker->getRaster();
* 
* 
* 
* 
* //mosaicing temporary files
* auto mosaicer = CsmMakerBuilder::fromMosaic(CsmMaker::mergeByMax).apply<CsmSmoother>(3);
* for (const auto& filename : tempCsmFilenames) {
*   mosaicer.addRaster(filename);
* }
* std::unique_ptr<CsmMaker> maker = mosaicer.build(align);
* Raster<csm_t> csm = maker->getRaster();
*/

namespace lapis {
    class CsmMakerBuilder {
        class CsmMosaicBuilder;
    public:

        static CsmMakerBuilder maxHeight(coord_t footprintDiameterCsmXYUnits = 0);

        template<class Algo, class... Args>
        static CsmMakerBuilder fromAlgo(Args&&... args);

        static CsmMosaicBuilder fromMosaic(const CsmMaker::CsmMergeFunc& mergeFunc);

        std::unique_ptr<CsmMaker> build(const Alignment& a) const;

        CsmMaker::CsmMergeFunc mergeFunction() const;

        template<class Decorator, class... Args>
        CsmMakerBuilder& apply(Args&&... args);

    private:
        CsmMakerBuilder() = default;

        std::function<std::unique_ptr<CsmMaker>(const Alignment&)> _buildFunc;

        class CsmMosaicBuilder {
        public:
            CsmMosaicBuilder(const CsmMaker::CsmMergeFunc& mergeFunc);

            CsmMosaicBuilder& addRaster(const std::string& filename);
            CsmMosaicBuilder& addRaster(const std::filesystem::path& filename);

            CsmMosaicBuilder& addRaster(const std::string& filename, const Alignment& a);
            CsmMosaicBuilder& addRaster(const std::filesystem::path& filename, const Alignment& a);

            CsmMosaicBuilder& addRaster(const Raster<csm_t>& r);
            CsmMosaicBuilder& addRaster(Raster<csm_t>&& r);

            CsmMosaicBuilder& assumeAllRastersAlign();

            std::unique_ptr<CsmMaker> build(const Alignment& a) const;

            template<class Decorator, class... Args>
            CsmMosaicBuilder& apply(Args&&... args);
        private:
            std::function<std::unique_ptr<CsmMaker>(std::unique_ptr<CsmMaker> prevMaker)> _applyDecorators;
            CsmMosaicerFactory _mosaicer;
            bool _assumeAllRastersAlign = false;
            CsmMaker::CsmMergeFunc _mergeFunc = CsmMaker::mergeByMean;
        };
    };
    template<class Algo, class ...Args>
    inline CsmMakerBuilder CsmMakerBuilder::fromAlgo(Args&&... args)
    {
        CsmMakerBuilder out;
        out._buildFunc = [...capturedArgs = std::forward<Args>(args)](const Alignment& a) mutable {
            return std::make_unique<Algo>(a, std::forward<decltype(capturedArgs)>(capturedArgs)...);
            };
        return out;
    }

    template<class Decorator, class ...Args>
    inline CsmMakerBuilder& CsmMakerBuilder::apply(Args&&... args)
    {
        auto oldBuildFunc = std::move(_buildFunc);
        _buildFunc = [oldBuildFunc = std::move(oldBuildFunc),
            ...capturedArgs = std::forward<Args>(args)](const Alignment& a) mutable {
            return std::make_unique<Decorator>(
                oldBuildFunc(a),
                std::forward<decltype(capturedArgs)>(capturedArgs)...
            );
            };
        return *this;
    }

    template<class Decorator, class ...Args>
    inline CsmMakerBuilder::CsmMosaicBuilder& CsmMakerBuilder::CsmMosaicBuilder::apply(Args&&... args)
    {
        if (!_applyDecorators) {
            _applyDecorators = [...capturedArgs = std::forward<Args>(args)](std::unique_ptr<CsmMaker> prevMaker) mutable {
                return std::make_unique<Decorator>(
                    std::move(prevMaker),
                    std::forward<decltype(capturedArgs)>(capturedArgs)...
                );
            };

        }
        else {
            auto oldApplyFunc = std::move(_applyDecorators);
            _applyDecorators = [oldApplyFunc = std::move(oldApplyFunc),
                ...capturedArgs = std::forward<Args>(args)](std::unique_ptr<CsmMaker> prevMaker) mutable {
                return std::make_unique<Decorator>(
                    oldApplyFunc(std::move(prevMaker)),
                    std::forward<decltype(capturedArgs)>(capturedArgs)...
                );
                };
        }
        return *this;
    }
}


#endif