#ifndef AL_VULKAN_H
#define AL_VULKAN_H

#include "engine/types.h"
#include "engine/platform/platform.h"
#include "engine/utilities/utilities.h"

#include "vulkan_base.h"
#include "vulkan_utils.h"
#include "vulkan_utils_initializers.h"
#include "vulkan_utils_converters.h"
#include "vulkan_memory_manager.h"
#include "vulkan_gpu.h"
#include "vulkan_command_buffers.h"
#include "vulkan_memory_buffer.h"

#include "../renderer_backend_core.h"
#include "texture_vulkan.h"
#include "framebuffer_vulkan.h"
#include "render_stage_vulkan.h"

namespace al
{
    struct SwapChain
    {
        VkSwapchainKHR handle;
        VkSurfaceFormatKHR surfaceFormat;
        VkExtent2D extent;
        Array<Texture*> images;
    };

    struct CommandPool
    {
        VkCommandPool handle;
        VkQueueFlags queueFlags;
    };

    struct CommandBuffer
    {
        VkCommandBuffer handle;
        VkQueueFlags queueFlags;
    };

    struct VulkanRendererBackend
    {
        using ShaderProgramStorage = DataBlockStorage<VulkanShaderProgram, 8>;
        using RenderStageStorage = DataBlockStorage<VulkanRenderStage, 8>;
        using TextureStorage = DataBlockStorage<VulkanTexture, 8>;
        using FramebufferStorage = DataBlockStorage<VulkanFramebuffer, 8>;

        PlatformWindow* window;
        VulkanMemoryManager memoryManager;
        VkInstance instance;
        VkDebugUtilsMessengerEXT debugMessenger;
        VkSurfaceKHR surface;
        VulkanGpu gpu;
        SwapChain swapChain;
        Array<VulkanCommandPoolSet> commandPoolSets;
        Array<VulkanCommandBufferSet> commandBufferSets;

        StaticPointerWithSize<VkSemaphore, utils::MAX_IMAGES_IN_FLIGHT> imageAvailableSemaphores;
        StaticPointerWithSize<VkSemaphore, utils::MAX_IMAGES_IN_FLIGHT> renderFinishedSemaphores;
        StaticPointerWithSize<VkFence, utils::MAX_IMAGES_IN_FLIGHT> inFlightFences;
        Array<VkFence> imageInFlightFencesRef;

        u32 activeRenderFrame;
        u32 activeSwapChainImageIndex;
        VulkanRenderStage* activeRenderStage;

        ShaderProgramStorage shaderPrograms;
        RenderStageStorage renderStages;
        TextureStorage textures;
        FramebufferStorage framebuffers;

        // DataBlockStorage<RenderPipeline, 8> renderPipelines;
    };

    void vulkan_backend_fill_vtable(RendererBackendVtable* table);
    RendererBackend* vulkan_backend_create(RendererBackendCreateInfo* createInfo);
    void vulkan_backend_destroy(RendererBackend* backend);
    void vulkan_backend_handle_resize(RendererBackend* backend);
    void vulkan_backend_begin_frame(RendererBackend* backend);
    void vulkan_backend_end_frame(RendererBackend* backend);
    PointerWithSize<Texture*> vulkan_backend_get_swap_chain_textures(RendererBackend* backend);
    uSize vulkan_backend_get_active_swap_chain_texture_index(RendererBackend* backend);
}

#endif
