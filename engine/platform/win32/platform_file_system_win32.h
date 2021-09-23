#ifndef AL_PLATFORM_FILE_SYSTEM_WIN32_H
#define AL_PLATFORM_FILE_SYSTEM_WIN32_H

#include "platform_win32_backend.h"
#include "../platform_file_system_config.h"
#include "../platform_file_system.h"

namespace al
{
    struct PlatformFile
    {
        using FlagsT = u64;
        struct Flags { enum : FlagsT {
            STD_IO      = FlagsT(1) << 0,
            IS_LOADED   = FlagsT(1) << 1,
        }; };
        HANDLE handle;
        PlatformFileMode mode;
        FlagsT flags;
    };
}

#endif
