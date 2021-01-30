#ifndef AL_PLATFORM_FILE_SYSTEM_UTILITIES_H
#define AL_PLATFORM_FILE_SYSTEM_UTILITIES_H

#include <string_view>

#include "platform_file_system_config.h"
#include "engine/containers/containers.h"

namespace al::engine
{
    template<typename ... Args>
    String construct_path(Args ... args) noexcept
    {
        String result;
        int count = sizeof...(Args);
        int unpack[]{0, (result += args, count -= 1, (count > 0) ? result += AL_PATH_SEPARATOR : result, 0)...};
        return result;
    }
}

#endif
