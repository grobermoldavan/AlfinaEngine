#ifndef AL_PLATFORM_FILE_SYSTEM_CONFIG_H
#define AL_PLATFORM_FILE_SYSTEM_CONFIG_H

#ifdef _WIN32
#   define AL_PATH_SEPARATOR "\\"
#else
#   error Unsupported platform
#endif

#endif
