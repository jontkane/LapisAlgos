#pragma once
#ifndef CSMSMOOTHER_HPP
#define CSMSMOOTHER_HPP

#include"algos_pch.hpp"
#include"CsmMaker.hpp"

namespace lapis {
    class CsmSmoother : public CsmMaker {
    public:
        CsmSmoother(std::unique_ptr<CsmMaker> csm, int smoothWindow);
        Raster<csm_t> getRaster() const override;
        void addPoints(const std::span<const LasPoint>& points) override;
        CsmMergeFunc getMergeFunction() const override;
    private:
        std::unique_ptr<CsmMaker> _csm;
        int _smoothWindow;
    };
}

#endif