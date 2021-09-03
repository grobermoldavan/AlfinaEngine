#ifndef AL_RENDER_PIPELINE_VULKAN_H
#define AL_RENDER_PIPELINE_VULKAN_H

#include "../base/render_pipeline.h"
#include "vulkan_base.h"

namespace al
{
    struct RenderDeviceVulkan;

    struct VulkanDescriptorSetLayout
    {
        struct Pool
        {
            static constexpr u32 MAX_SETS = 64;
            VkDescriptorPool handle;
            uSize numAllocations;
            bool isLastAllocationSuccessful;
        };
        DynamicArray<Pool> pools;
        Array<VkDescriptorPoolSize> poolSizes;
        VkDescriptorPoolCreateInfo poolCreateInfo;
        VkDescriptorSetLayout handle;
    };

    struct DescriptorSetVulkan : DescriptorSet
    {
        VkDescriptorSet handle;
    };

    struct RenderPipelineVulkan : RenderPipeline
    {
        Array<VulkanDescriptorSetLayout> descriptorSetLayouts;
        VkPipelineLayout layoutHandle;
        VkPipeline handle;
        VkPipelineBindPoint bindPoint;
        RenderDeviceVulkan* device;
    };

    RenderPipeline* vulkan_render_pipeline_graphics_create(GraphicsRenderPipelineCreateInfo* createInfo);
    void vulkan_render_pipeline_destroy(RenderPipeline* pipeline);
}

#endif
