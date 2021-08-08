#ifndef AL_RENDER_PROGRAM_VULKAN_H
#define AL_RENDER_PROGRAM_VULKAN_H

#include "../base/render_program.h"
#include "../base/spirv_reflection/spirv_reflection.h"
#include "vulkan_base.h"

namespace al
{
    struct RenderDeviceVulkan;

    struct RenderProgramVulkan : RenderProgram
    {
        static constexpr uSize REFLECTION_BUFFER_SIZE_BYTES = 1024;
        SpirvReflection reflection;
        RenderDeviceVulkan* device;
        VkShaderModule handle;
    };

    RenderProgram* vulkan_render_program_create(RenderProgramCreateInfo* createInfo);
    void vulkan_render_program_destroy(RenderProgram* program);
}

#endif
