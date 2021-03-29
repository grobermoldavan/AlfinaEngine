#ifndef AL_PLATFORM_H
#define AL_PLATFORM_H

#include "engine/platform/platform_input.h"
#include "engine/platform/platform_window.h"

#ifdef _WIN32
#   include "engine/platform/win32/platform_input_win32.h"
#   include "engine/platform/win32/platform_window_win32.h"
#else
#   error Unsupported platform
#endif

#endif
