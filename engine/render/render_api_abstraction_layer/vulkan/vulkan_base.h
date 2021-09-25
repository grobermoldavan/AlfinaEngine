#ifndef AL_VULKAN_BASE_H
#define AL_VULKAN_BASE_H

#ifdef _WIN32
#   define VK_USE_PLATFORM_WIN32_KHR
#else
#   error Unsupported platform
#endif

#include <vulkan/vulkan.h>
#include <cassert>
#include <cstring>

#include "engine/types.h"
#include "engine/assert/assert.h"
#include "engine/memory/memory.h"
#include "engine/utilities/utilities.h"
#include "engine/platform/platform.h"

#define al_vk_check(cmd)                do { VkResult __result = cmd; assert(__result == VK_SUCCESS); } while(0)
// #define al_vk_assert(cmd)               assert(cmd)
// #define al_vk_assert_msg(cmd, msg)      assert(cmd && msg)
// #define al_vk_assert_fail(msg)          assert(!msg)
#define al_vk_strcmp(str1, str2)        (::std::strcmp(str1, str2) == 0)
#define al_vk_memcpy(dst, src, size)    do { ::std::memcpy(dst, src, size); } while(0)
#define al_vk_memset(dst, val, size)    do { ::std::memset(dst, val, size); } while(0)

#endif
