#ifndef AL_RENDER_COMMAND_BUFFER_VULKAN_H
#define AL_RENDER_COMMAND_BUFFER_VULKAN_H

#include "../base/command_buffer.h"
#include "vulkan_base.h"

namespace al
{
    struct RenderDeviceVulkan;
    struct TextureVulkan;

    struct CommandBufferVulkan : CommandBuffer
    {
        using FlagsT = u64;
        struct Flags { enum : FlagsT {
            HAS_SUBMITTED_COMMANDS = FlagsT(1) << 0,
            HAS_STARTED_RENDER_PASS = FlagsT(1) << 1,
        }; };
        RenderDeviceVulkan* device;
        VkCommandBuffer handle;
        CommandBufferUsage usage;
        VkSemaphore executionFinishedSemaphore;
        VkFence executionFence;
        FlagsT flags;
    };

    CommandBuffer* vulkan_command_buffer_request(CommandBufferRequestInfo* requestInfo);
    void vulkan_command_buffer_submit(CommandBuffer* buffer);
    void vulkan_command_buffer_bind_pipeline(CommandBuffer* buffer, CommandBindPipelineInfo* commandInfo);
    void vulkan_command_buffer_draw(CommandBuffer* buffer, CommandDrawInfo* commandInfo);
    
    void vulkan_command_buffer_destroy(CommandBufferVulkan* buffer);
    void vulkan_command_buffer_transition_image_layout(CommandBufferVulkan* buffer, TextureVulkan* texture, VkImageLayout targetLayout);
}

#endif
