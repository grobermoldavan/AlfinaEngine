#ifndef AL_CONTAINERS_H
#define AL_CONTAINERS_H

#include <vector>

#include "engine/memory/pool_allocator_std_wrap.h"

namespace al::engine
{
    // @TODO :  make dynamic array which does not throw exceptions
    template<typename T>
    using DynamicArray = std::vector<T, PoolAllocatorStdWrap<T>>;
}

#endif
