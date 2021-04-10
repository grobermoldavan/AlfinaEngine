#ifndef AL_PLATFORM_FILE_SYSTEM_H
#define AL_PLATFORM_FILE_SYSTEM_H

#include "engine/config.h"
#include "engine/types.h"
#include "engine/memory/memory.h"

namespace al
{
    struct PlatformFilePath
    {
        char memory[EngineConfig::PLATFORM_FILE_PATH_SIZE];
    };

    struct PlatformFile
    {
        uSize sizeBytes;
        uSize sizeBytesNoAlign;
        void* memory;
    };

    enum class PlatformFileLoadMode
    {
        READ,
        WRITE
    };

    bool platform_file_path_append(PlatformFilePath* path, const char* string);

    template<typename ... Args>
    PlatformFilePath platform_file_path_construct(Args ... args);

    PlatformFile platform_file_load(AllocatorBindings bindings, const PlatformFilePath& path, PlatformFileLoadMode loadMode);
    void platform_file_unload(AllocatorBindings bindings, PlatformFile file);
}

#endif
