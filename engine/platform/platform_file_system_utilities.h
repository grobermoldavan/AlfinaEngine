#ifndef AL_PLATFORM_FILE_SYSTEM_UTILITIES_H
#define AL_PLATFORM_FILE_SYSTEM_UTILITIES_H

#include "platform_file_system_config.h"
#include "engine/containers/containers.h"

namespace al::engine
{
    template<typename ... Args>
    StaticString construct_path(Args ... args) noexcept
    {
        StaticString result;
        construct(&result);
        int count = sizeof...(Args);
        bool appendResult = true;
        int unpack[]
        {
            0, (appendResult = appendResult && append(&result, args), 
            count -= 1, 
            (count > 0) ? appendResult = appendResult && append(&result, AL_PATH_SEPARATOR) : true, 0)...
        };
        // StaticString is not long enough to hold full path 
        al_assert(appendResult);
        return result;
    }
}

#endif
