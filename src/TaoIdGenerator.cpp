#include"TaoIdGenerator.hpp"

namespace lapis {
    TaoIdGenerator::TaoIdGenerator(taoid_t startAt, taoid_t increaseBy)
        : _currentId(startAt), _increaseBy(increaseBy)
    {}
    taoid_t TaoIdGenerator::nextId()
    {
        taoid_t toReturn = _currentId;
        _currentId += _increaseBy;
        return toReturn;
    }
}