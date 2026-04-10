#pragma once
#ifndef TAOID_HPP
#define TAOID_HPP

#include"algos_pch.hpp"
#include"algos_types.hpp"
#include"TaoIdGenerator.hpp"

namespace lapis {
    class TaoIdAlgo {
    public:
        virtual ~TaoIdAlgo() noexcept = default;
        virtual std::vector<IDedTao> process(const Raster<csm_t>& bufferedCsm, TaoIdGenerator& idGen) const = 0;
    };
}

#endif