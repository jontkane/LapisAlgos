#pragma once
#ifndef CSMBASE_HPP
#define CSMBASE_HPP

#include"algos_pch.hpp"
#include"algos_types.hpp"

namespace lapis {
    class CsmMaker {
    public:
        virtual ~CsmMaker() = default;
        virtual Raster<csm_t> getRaster() const = 0;
        virtual void addPoints(const std::span<const LasPoint>& points) = 0;

        //no bounds checking
        virtual void addPointsUnsafe(const std::span<const LasPoint>& points) = 0;

        using CsmMergeFunc = std::function<csm_t(csm_t, csm_t)>;
        virtual CsmMergeFunc getMergeFunction() const = 0;

        static csm_t mergeByMax(csm_t a, csm_t b) {
            return std::max(a, b);
        }
        static csm_t mergeByMin(csm_t a, csm_t b) {
            return std::min(a, b);
        }
        static csm_t mergeByMean(csm_t a, csm_t b) {
            return (a + b) / 2.;
        }
    };
}


#endif