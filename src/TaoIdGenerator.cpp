#include"TaoIdGenerator.hpp"

namespace lapis {
    taoid_t SequentialTaoIdGenerator::nextId()
    {
        _currentId++;
        return _currentId;
    }
    ByTileTaoIdGenerator::ByTileTaoIdGenerator(size_t tileCount, size_t thisTileIndex)
    {
        _tileCount = (taoid_t)tileCount;
        _currentId = (taoid_t)thisTileIndex;
        if (_currentId == 0) {
            _currentId = (taoid_t)tileCount;
        }
    }
    taoid_t ByTileTaoIdGenerator::nextId()
    {
        taoid_t id = _currentId;
        _currentId += _tileCount;
        return id;
    }
}