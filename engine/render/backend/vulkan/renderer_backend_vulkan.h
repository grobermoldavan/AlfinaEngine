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
#include "vulkan_memory_buffer.h"

#include "../renderer_backend_core.h"
#include "texture_vulkan.h"
#include "framebuffer_vulkan.h"
#include "render_stage_vulkan.h"

namespace al
{
    struct SwapChain
    {
        struct Image
        {
            VkImage handle;
            VkImageView view;
        };
        VkSwapchainKHR handle;
        VkSurfaceFormatKHR surfaceFormat;
        VkExtent2D extent;
        Array<Image> images;
    };

    struct CommandPool
    {
        using FlagsT = u32;
        enum Flags : FlagsT
        {
            GRAPHICS = FlagsT(1) << 0,
            TRANSFER = FlagsT(1) << 1,
        };
        VkCommandPool handle;
        FlagsT flags;
    };

    struct VulkanRendererBackend
    {
        PlatformWindow* window;
        VulkanMemoryManager memoryManager;
        VkInstance instance;
        VkDebugUtilsMessengerEXT debugMessenger;
        VkSurfaceKHR surface;
        VulkanGpu gpu;
        SwapChain swapChain;
        Array<CommandPool> commandPools;
        Array<VkCommandBuffer> graphicsBuffers;
        VkCommandBuffer transferBuffer;

        StaticPointerWithSize<VkSemaphore, utils::MAX_IMAGES_IN_FLIGHT> imageAvailableSemaphores;
        StaticPointerWithSize<VkSemaphore, utils::MAX_IMAGES_IN_FLIGHT> renderFinishedSemaphores;
        StaticPointerWithSize<VkFence, utils::MAX_IMAGES_IN_FLIGHT> inFlightFences;
        Array<VkFence> imageInFlightFencesRef;

        u32 activeRenderFrame;
        u32 activeSwapChainImageIndex;

        DataBlockStorage<VulkanShaderProgram, 8> shaderPrograms;
        DataBlockStorage<VulkanRenderStage, 8> shaderStages;
        DataBlockStorage<VulkanTexture, 8> textures;
        DataBlockStorage<VulkanFramebuffer, 8> framebuffers;

        // DataBlockStorage<RenderPipeline, 8> renderPipelines;
    };

    void vulkan_backend_fill_vtable(RendererBackendVtable* table);
    
    RendererBackend* vulkan_backend_create(RendererBackendCreateInfo* createInfo);
    void vulkan_backend_destroy(RendererBackend* backend);
    void vulkan_backend_handle_resize(RendererBackend* backend);
    void vulkan_backend_begin_frame(RendererBackend* backend);
    void vulkan_backend_end_frame(RendererBackend* backend);
}

#endif
