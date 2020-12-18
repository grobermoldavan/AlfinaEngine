#ifndef AL_CONTAINERS_H
#define AL_CONTAINERS_H

#include <vector>

#include "engine/memory/pool_allocator.h"

namespace al::engine
{
    template<typename T>
    using DynamicArray = std::vector<T, PoolAllocatorStdWrap<T>>;
}

#endif
