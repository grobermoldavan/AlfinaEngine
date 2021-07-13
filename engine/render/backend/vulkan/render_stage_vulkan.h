#ifndef AL_RENDER_STAGE_VULKAN_H
#define AL_RENDER_STAGE_VULKAN_H

#include "../render_stage_base.h"
#include "vulkan_base.h"

namespace al
{
    struct VulkanShaderProgram : ShaderProgram
    {
        VkShaderModule handle;
    };

    struct VulkanRenderStage : RenderStage
    {
        VkRenderPass renderPass;
        VkPipelineLayout pipelineLayout;
        VkPipeline pipeline;
    };

    ShaderProgram* vulkan_shader_program_create(RendererBackend* backend, PlatformFile bytecode);
    void vulkan_shader_program_destroy(RendererBackend* backend, ShaderProgram* program);

    RenderStage* vulkan_render_stage_create(RendererBackend* backend, RenderStageCreateInfo* createInfo);
    void vulkan_render_stage_destroy(RendererBackend* backend, RenderStage* stage);
}

#endif
