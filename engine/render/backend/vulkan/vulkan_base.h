#ifndef AL_VULKAN_CORE_H
#define AL_VULKAN_CORE_H

#ifdef _WIN32
#   define VK_USE_PLATFORM_WIN32_KHR
#else
#   error Unsupported platform
#endif

#include <vulkan/vulkan.h>
#include <cassert>
#include <cstring>

#define al_vk_check(cmd)                do { VkResult result = cmd; assert(result == VK_SUCCESS); } while(0)
#define al_vk_assert(cmd)               assert(cmd)
#define al_vk_strcmp(str1, str2)        (::std::strcmp(str1, str2) == 0)
#define al_vk_memcpy(dst, src, size)    do { ::std::memcpy(dst, src, size); } while(0)
#define al_vk_memset(dst, val, size)    do { ::std::memset(dst, val, size); } while(0)

#endif