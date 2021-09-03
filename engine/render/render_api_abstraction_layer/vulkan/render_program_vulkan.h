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
        SpirvReflection reflection;
        RenderDeviceVulkan* device;
        VkShaderModule handle;
    };

    struct VulkanCombinedProgramInfo
    {
        Array<VkDescriptorSetLayoutCreateInfo> descriptorSetLayoutCreateInfos;
        Array<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings;
    };

    RenderProgram* vulkan_render_program_create(RenderProgramCreateInfo* createInfo);
    void vulkan_render_program_destroy(RenderProgram* program);

    VulkanCombinedProgramInfo vulkan_combined_program_info_create(PointerWithSize<RenderProgramVulkan*> programs, AllocatorBindings* persistentAllocator);
    void vulkan_combined_program_info_destroy(VulkanCombinedProgramInfo* combinedInfo);
}

#endif
