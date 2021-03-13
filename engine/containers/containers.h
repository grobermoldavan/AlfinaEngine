#ifndef AL_CONTAINERS_H
#define AL_CONTAINERS_H

#include <string>

#include "engine/config/engine_config.h"
#include "engine/memory/pool_allocator_std_wrap.h"
#include "dynamic_array.h"
#include "array_view.h"

#include "utilities/fixed_size_string.h"

namespace al::engine
{
    using String        = std::basic_string<char, std::char_traits<char>, PoolAllocatorStdWrap<char>>;
    using StaticString  = FixedSizeString<EngineConfig::STATIC_STRING_LENGTH>;

    StaticString construct_static_string(const char* cstr = "")
    {
        StaticString result;
        construct(&result, cstr);
        return result;
    }
}

#endif
