#ifndef AL_PLATFORM_FILE_SYSTEM_H
#define AL_PLATFORM_FILE_SYSTEM_H

#include "engine/config.h"
#include "engine/types.h"
#include "engine/memory/memory.h"
#include "engine/result.h"

namespace al
{
    struct PlatformFile;

    enum struct PlatformFileMode : u64
    {
        READ,
        WRITE
    };

    struct PlatformFilePath
    {
        char memory[EngineConfig::PLATFORM_FILE_PATH_SIZE];
    };

    struct PlatformFileContent
    {
        AllocatorBindings allocator;
        void* memory;
        u64 sizeBytes;
    };

    bool platform_file_path_append(PlatformFilePath* path, const char* string);

    template<typename ... Args>
    [[nodiscard]] Result<PlatformFilePath> platform_path(Args ... args);

    [[nodiscard]] Result<void>                  platform_file_get_std_out   (PlatformFile* file);
    [[nodiscard]] Result<void>                  platform_file_load          (PlatformFile* file, const PlatformFilePath& path, PlatformFileMode loadMode);
                  Result<void>                  platform_file_unload        (PlatformFile* file);

    [[nodiscard]] Result<PlatformFileContent>   platform_file_read          (PlatformFile* file, AllocatorBindings* allocator);
                  Result<void>                  platform_file_free_content  (PlatformFileContent* content);
    
    [[nodiscard]] Result<void>                  platform_file_write         (PlatformFile* file, const void* data, uSize dataSizeBytes);
}

#endif
