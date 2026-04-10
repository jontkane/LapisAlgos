#pragma once
#ifndef ALGOS_TYPES_HPP
#define ALGOS_TYPES_HPP

#include<cstdint>

namespace lapis {
    using csm_t = double;
    using taoid_t = uint32_t;

    struct IDedTao {
        taoid_t id;
        cell_t location;
    };
}

#endif