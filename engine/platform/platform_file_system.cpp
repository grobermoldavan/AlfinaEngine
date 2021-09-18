
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
    [[nodiscard]] Result<PlatformFilePath> platform_path(Args ... args)
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

        dbg (if (!appendResult) return err<PlatformFilePath>("Can't build platform path - path is too long"));
        return ok<PlatformFilePath>(result);
    }
}
