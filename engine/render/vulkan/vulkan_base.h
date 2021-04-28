#ifndef AL_VULKAN_CORE_H
#define AL_VULKAN_CORE_H

#ifdef _WIN32
#   define VK_USE_PLATFORM_WIN32_KHR
#else
#   error Unsupported platform
#endif

#include <vulkan/vulkan.h>

#include <cassert>

#define al_vk_check(cmd) do { VkResult result = cmd; assert(result == VK_SUCCESS); } while(0)

#endif
