#pragma once
#ifndef TAOIDGENERATOR_HPP
#define TAOIDGENERATOR_HPP

#include"algos_pch.hpp"
#include"algos_types.hpp"

namespace lapis {
    class TaoIdGenerator {
    public:
        TaoIdGenerator(taoid_t startAt, taoid_t increaseBy);
        taoid_t nextId();
    private:
        taoid_t _currentId;
        taoid_t _increaseBy;
    };
}

#endif