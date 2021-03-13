#ifndef AL_FILE_LOAD_H
#define AL_FILE_LOAD_H

#include <cstddef>
#include <stdio.h>
#include <string_view>

#include "engine/debug/debug.h"
#include "engine/memory/memory_common.h"
#include "engine/memory/allocator_base.h"

namespace al::engine
{
    enum class FileLoadMode
    {
        READ,
        WRITE
    };

    struct al_align FileHandle
    {
        enum class State
        {
            FREE,
            LOADED,
            LOADING
        };
        std::size_t size;
        State state;
        std::byte* memory;
    };

    static const char* LOAD_MODE_TO_STR[] =
    {
        "rb",
        "wb"
    };

    FileHandle sync_load(const char* name, AllocatorBase* allocator, FileLoadMode mode);
}

#endif
