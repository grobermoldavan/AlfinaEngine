#ifndef AL_RENDER_PIPELINE_VULKAN_H
#define AL_RENDER_PIPELINE_VULKAN_H

#include "../base/render_pipeline.h"
#include "vulkan_base.h"

namespace al
{
    struct RenderPipelineVulkan : RenderPipeline
    {
        VkPipelineLayout pipelineLayout;
        VkPipeline pipeline;
        VkPipelineBindPoint bindPoint;
    };

    RenderPipeline* vulkan_render_pipeline_graphics_create(GraphicsRenderPipelineCreateInfo* createInfo);
    void vulkan_render_pipeline_destroy(RenderPipelineVulkan* pipeline);
}

#endif
