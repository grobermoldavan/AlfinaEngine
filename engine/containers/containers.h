#ifndef AL_CONTAINERS_H
#define AL_CONTAINERS_H

#include <vector>
#include <string>

#include "engine/config/engine_config.h"
#include "engine/memory/pool_allocator_std_wrap.h"

#include "utilities/fixed_size_string.h"

namespace al::engine
{
    // @TODO :  make dynamic array which does not throw exceptions
    template<typename T>
    using DynamicArray = std::vector<T, PoolAllocatorStdWrap<T>>;
    
    using String = std::basic_string<char, std::char_traits<char>, PoolAllocatorStdWrap<char>>;

    using StaticString = FixedSizeString<EngineConfig::STATIC_STRING_LENGTH>;
}

#endif
