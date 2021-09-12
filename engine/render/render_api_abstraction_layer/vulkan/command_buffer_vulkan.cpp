
#include "command_buffer_vulkan.h"
#include "render_device_vulkan.h"
#include "render_pipeline_vulkan.h"
#include "framebuffer_vulkan.h"
#include "vulkan_utils.h"

namespace al
{
    VulkanGpu::CommandQueueFlags vulkan_buffer_usage_to_queue_flags(CommandBufferUsage usage)
    {
        switch (usage)
        {
            case CommandBufferUsage::GRAPHICS: return VulkanGpu::CommandQueue::GRAPHICS;
            case CommandBufferUsage::TRANSFER: return VulkanGpu::CommandQueue::TRANSFER;
            // case CommandBufferUsage::COMPUTE: return VulkanGpu::CommandQueue::COMPUTE;
        }
        al_vk_assert_fail("Unsupported CommandBufferUsage");
        return VulkanGpu::CommandQueueFlags(0);
    };

    CommandBuffer* vulkan_command_buffer_request(CommandBufferRequestInfo* requestInfo)
    {
        // TODO : make pool of CommandBufferVulkan objects
        RenderDeviceVulkan* device = (RenderDeviceVulkan*)requestInfo->device;
        CommandBufferVulkan* buffer = allocate<CommandBufferVulkan>(&device->memoryManager.cpu_persistentAllocator);

        const VulkanGpu::CommandQueueFlags queueFlags = vulkan_buffer_usage_to_queue_flags(requestInfo->usage);
        VkCommandBufferAllocateInfo allocateInfo
        {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext              = nullptr,
            .commandPool        = get_command_queue(&device->gpu, queueFlags)->commandPoolHandle,
            .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        al_vk_check(vkAllocateCommandBuffers(device->gpu.logicalHandle, &allocateInfo, &buffer->handle));
        VkSemaphoreCreateInfo semaphoreCreateInfo
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
        };
        al_vk_check(vkCreateSemaphore(device->gpu.logicalHandle, &semaphoreCreateInfo, &device->memoryManager.cpu_allocationCallbacks, &buffer->executionFinishedSemaphore));
        VkFenceCreateInfo fenceCreateInfo
        {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
        };
        al_vk_check(vkCreateFence(device->gpu.logicalHandle, &fenceCreateInfo, &device->memoryManager.cpu_allocationCallbacks, &buffer->executionFence));
        buffer->device = device;
        buffer->usage = requestInfo->usage;
        buffer->flags = 0;

        VkCommandBufferBeginInfo beginInfo
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = nullptr,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            .pInheritanceInfo = nullptr,
        };
        al_vk_check(vkBeginCommandBuffer(buffer->handle, &beginInfo));

        return buffer;
    }

    void vulkan_command_buffer_submit(CommandBuffer* _buffer)
    {
        CommandBufferVulkan* buffer = (CommandBufferVulkan*)(_buffer);
        RenderDeviceVulkan* device = buffer->device;

        al_vk_assert_msg(buffer->flags & CommandBufferVulkan::Flags::HAS_SUBMITTED_COMMANDS, "Can't submit command buffer - no commands were provided");

        VulkanInFlightData::PerImageInFlightData* inFlightData = vulkan_in_flight_data_get_current(&device->inFlightData);
        const VulkanGpu::CommandQueueFlags queueFlags = vulkan_buffer_usage_to_queue_flags(buffer->usage);
        VulkanGpu::CommandQueue* queue = get_command_queue(&device->gpu, queueFlags);

        if (buffer->flags & CommandBufferVulkan::Flags::HAS_STARTED_RENDER_PASS)
        {
            vkCmdEndRenderPass(buffer->handle);
        }
        al_vk_check(vkEndCommandBuffer(buffer->handle));
        VkSemaphore waitSemaphores[] = { VK_NULL_HANDLE };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_ALL_COMMANDS_BIT }; // TODO : optimize this
        VkSubmitInfo submitInfo
        {
            .sType                  = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext                  = nullptr,
            .waitSemaphoreCount     = 0,
            .pWaitSemaphores        = waitSemaphores,
            .pWaitDstStageMask      = waitStages,
            .commandBufferCount     = 1,
            .pCommandBuffers        = &buffer->handle,
            .signalSemaphoreCount   = 1,
            .pSignalSemaphores      = &buffer->executionFinishedSemaphore,
        };
        if (inFlightData->commandBuffers.size)
        {
            CommandBufferVulkan* waitBuffer = inFlightData->commandBuffers[inFlightData->commandBuffers.size - 1];
            waitSemaphores[0] = waitBuffer->executionFinishedSemaphore;
            submitInfo.waitSemaphoreCount += 1;
        }
        al_vk_check(vkQueueSubmit(queue->handle, 1, &submitInfo, buffer->executionFence));

        CommandBufferVulkan** ptr = dynamic_array_add(&inFlightData->commandBuffers);
        *ptr = buffer;

        // TODO : Move this to some device function
        device->flags |= RenderDeviceVulkan::Flags::HAS_SUBMITTED_BUFFERS;
    }

    void vulkan_command_buffer_bind_pipeline(CommandBuffer* _buffer, CommandBindPipelineInfo* commandInfo)
    {
        CommandBufferVulkan* buffer = (CommandBufferVulkan*)(_buffer);
        RenderDeviceVulkan* device = buffer->device;
        RenderPipelineVulkan* pipeline = (RenderPipelineVulkan*)commandInfo->pipeline;
        FramebufferVulkan* framebuffer = (FramebufferVulkan*)commandInfo->framebuffer;
        RenderPassVulkan* renderPass = (RenderPassVulkan*)pipeline->renderPass;

        vulkan_framebuffer_prepare(framebuffer);

        VkRenderPassBeginInfo renderPassInfo
        {
            .sType              = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .pNext              = nullptr,
            .renderPass         = renderPass->handle,
            .framebuffer        = framebuffer->handle,
            .renderArea         = { .offset = { 0, 0 }, .extent = device->swapChain.extent },
            .clearValueCount    = u32(renderPass->clearValues.size),
            .pClearValues       = renderPass->clearValues.memory,
        };
        VkViewport viewport
        {
            .x          = 0.0f,
            .y          = 0.0f,
            .width      = float(device->swapChain.extent.width),
            .height     = float(device->swapChain.extent.height),
            .minDepth   = 0.0f,
            .maxDepth   = 1.0f,
        };
        VkRect2D scissor
        {
            .offset = { 0, 0 },
            .extent = device->swapChain.extent,
        };
        vkCmdSetViewport(buffer->handle, 0, 1, &viewport);
        vkCmdSetScissor(buffer->handle, 0, 1, &scissor);
        vkCmdBeginRenderPass(buffer->handle, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(buffer->handle, pipeline->bindPoint, pipeline->handle);

        buffer->flags |= CommandBufferVulkan::Flags::HAS_SUBMITTED_COMMANDS | CommandBufferVulkan::Flags::HAS_STARTED_RENDER_PASS;
    }

    void vulkan_command_buffer_draw(CommandBuffer* _buffer, CommandDrawInfo* commandInfo)
    {
        CommandBufferVulkan* buffer = (CommandBufferVulkan*)(_buffer);
        vkCmdDraw(buffer->handle, commandInfo->vertexCount, 1, 0, 0);

        buffer->flags |= CommandBufferVulkan::Flags::HAS_SUBMITTED_COMMANDS;
    }

    void vulkan_command_buffer_destroy(CommandBufferVulkan* buffer)
    {
        RenderDeviceVulkan* device = buffer->device;
        const VulkanGpu::CommandQueueFlags queueFlags = vulkan_buffer_usage_to_queue_flags(buffer->usage);
        VulkanGpu::CommandQueue* queue = get_command_queue(&device->gpu, queueFlags);

        vkDestroySemaphore(device->gpu.logicalHandle, buffer->executionFinishedSemaphore, &device->memoryManager.cpu_allocationCallbacks);
        vkDestroyFence(device->gpu.logicalHandle, buffer->executionFence, &device->memoryManager.cpu_allocationCallbacks);
        vkFreeCommandBuffers(device->gpu.logicalHandle, queue->commandPoolHandle, 1, &buffer->handle);
        deallocate<CommandBufferVulkan>(&device->memoryManager.cpu_persistentAllocator, buffer);
    }

    void vulkan_command_buffer_transition_image_layout(CommandBufferVulkan* buffer, TextureVulkan* texture, VkImageLayout targetLayout)
    {
        VkImageMemoryBarrier imageBarrier
        {
            .sType                  = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext                  = nullptr,
            .srcAccessMask          = utils::image_layout_to_access_flags(texture->currentLayout),
            .dstAccessMask          = utils::image_layout_to_access_flags(targetLayout),
            .oldLayout              = texture->currentLayout,
            .newLayout              = targetLayout,
            .srcQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
            .image                  = texture->image,
            .subresourceRange       = texture->subresourceRange,
        };
        vkCmdPipelineBarrier
        (
            buffer->handle,
            utils::image_layout_to_pipeline_stage_flags(texture->currentLayout),
            utils::image_layout_to_pipeline_stage_flags(targetLayout),
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &imageBarrier
        );
        texture->currentLayout = targetLayout;
        
        buffer->flags |= CommandBufferVulkan::Flags::HAS_SUBMITTED_COMMANDS;
    }
}
