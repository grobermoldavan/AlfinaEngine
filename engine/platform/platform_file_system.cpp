
#include "engine/config.h"
#include "engine/platform/platform_file_system.h"
#include "engine/platform/platform_file_system_config.h"

#include <cstring>  // for std::memcpy, std::memset, std::strlen
#include <cstdio>

namespace al
{
    bool platform_file_path_append(PlatformFilePath* path, const char* string)
    {
        if (!string)
        {
            return false;
        }
        uSize currentLength = std::strlen(path->memory);
        uSize appendLength  = std::strlen(string);
        if ((currentLength + appendLength) > (EngineConfig::PLATFORM_FILE_PATH_SIZE - 1))
        {
            // Not enough space
            return false;
        }
        std::memcpy(path->memory + currentLength, string, appendLength);
        return true;
    }

    template<typename ... Args>
    PlatformFilePath platform_path(Args ... args)
    {
        PlatformFilePath result{ };
        s32 count = sizeof...(Args);
        bool appendResult = true;
        s32 unpack[]
        {
            0, (appendResult = appendResult && platform_file_path_append(&result, args), 
            count -= 1, 
            (count > 0) ? appendResult = appendResult && platform_file_path_append(&result, AL_PATH_SEPARATOR) : true, 0)...
        };
        // PlatformFilePath is not long enough to store full path
        // @TODO :  add assert
        // assert(appendResult);
        return result;
    }

    PlatformFile platform_file_load(AllocatorBindings* bindings, const PlatformFilePath& path, PlatformFileLoadMode loadMode)
    {
        static const char* LOAD_MODE_TO_STR[] =
        {
            "rb",
            "wb"
        };
        std::FILE* file = std::fopen(path.memory, LOAD_MODE_TO_STR[static_cast<u32>(loadMode)]); // al_assert(file);
        std::fseek(file, 0, SEEK_END);
        uSize fileSize = std::ftell(file);
        std::fseek(file, 0, SEEK_SET);
        void* memory = allocate(bindings, fileSize + 1);
        std::memset(memory, 0, fileSize + 1);
        // al_assert(memory);
        uSize readSize = std::fread(memory, sizeof(u8), fileSize, file);
        // al_assert(readSize == (fileSize - 1));
        std::fclose(file);
        return
        {
            .sizeBytes  = fileSize,
            .memory     = memory
        };
    }

    void platform_file_unload(AllocatorBindings* bindings, PlatformFile file)
    {
        deallocate(bindings, file.memory, file.sizeBytes);
    }
}
