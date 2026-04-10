#pragma once
#ifndef TAOIDGENERATOR_HPP
#define TAOIDGENERATOR_HPP

#include"algos_pch.hpp"
#include"algos_types.hpp"

namespace lapis {
    class TaoIdGenerator {
    public:
        virtual ~TaoIdGenerator() = default;
        virtual taoid_t nextId() = 0;
    };

    class SequentialTaoIdGenerator : public TaoIdGenerator {
    public:
        taoid_t nextId() override;
    private:
        taoid_t _currentId = 0;
    };

    class ByTileTaoIdGenerator : public TaoIdGenerator {
    public:
        ByTileTaoIdGenerator(size_t tileCount, size_t thisTileIndex);
        taoid_t nextId() override;
    private:
        taoid_t _currentId;
        taoid_t _tileCount;
    };
}

#endif