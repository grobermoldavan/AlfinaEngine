#ifndef __ALFINA_ENGINE_H__
#define __ALFINA_ENGINE_H__

#define AL_UNITY_BUILD

#include "engine_utilities\asserts.h"
#include "engine_utilities\error_info.h"
#include "engine_utilities\string\string_utilities.h"

#include "engine_utilities\logging\logging.h"
#if defined(AL_PLATFORM_WIN32)
#	include "engine_utilities\logging\win32\windows_logging.h"
#else
#	error Supported platform is not defined
#endif

#include "math\math.h"

#include "platform\base_os_platform.h"
#if defined(AL_PLATFORM_WIN32)
#	include "platform\win32\windows_platform.h"
#else
#	error Supported platform is not defined
#endif

#include "platform\base_file_sys.h"
#if defined(AL_PLATFORM_WIN32)
#	include "platform\win32\windows_file_sys.h"
#else
#	error Supported platform is not defined
#endif

#include "renderer\base_renderer.h"
#if defined(AL_PLATFORM_WIN32)
#	include "renderer\win32\windows_opengl_renderer.h"
#else
#	error Supported platform is not defined
#endif

#include "sound_system\base_sound_system.h"
#if defined(AL_PLATFORM_WIN32)
#	include "sound_system\win32\windows_sound_system.h"
#else
#	error Supported platform is not defined
#endif

#endif
