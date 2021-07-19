
#include <cassert>
#include <cstdlib>
#include <cstring>

#include "engine/config.h"
#include "engine/utilities/utilities.h"
#include "renderer_backend_vulkan.h"

namespace al
{
    //
    // Instance functions
    //
    void construct_instance(VkInstance* instance, VkDebugUtilsMessengerEXT* debugMessenger, RendererBackendCreateInfo* backendCreateInfo, AllocatorBindings* bindings, VkAllocationCallbacks* callbacks);
    void destroy_instance(VkInstance instance, VkAllocationCallbacks* callbacks, VkDebugUtilsMessengerEXT messenger);
    //
    // Surface functions
    //
    void construct_surface(VkSurfaceKHR* surface, PlatformWindow* window, VkInstance instance, VkAllocationCallbacks* allocationCallbacks);
    void destroy_surface(VkSurfaceKHR surface, VkInstance instance, VkAllocationCallbacks* callbacks);
    //
    // Swap chain
    //
    void fill_swap_chain_sharing_mode(VkSharingMode* resultMode, u32* resultCount, PointerWithSize<u32> resultFamilyIndices, VulkanGpu* gpu);
    void construct_swap_chain(SwapChain* swapChain, VkSurfaceKHR surface, VulkanGpu* gpu, PlatformWindow* window, AllocatorBindings* bindings, VkAllocationCallbacks* callbacks);
    void destroy_swap_chain(SwapChain* swapChain, VulkanGpu* gpu, VkAllocationCallbacks* callbacks);
    void construct_swap_chain_images(SwapChain* swapChain, VulkanGpu* gpu, AllocatorBindings* bindings, VkAllocationCallbacks* callbacks, VulkanRendererBackend::TextureStorage* textureStorage);
    void destroy_swap_chain_images(SwapChain* swapChain, VulkanGpu* gpu, VulkanMemoryManager* memoryManager);
    // //
    // // Command pools
    // //
    // void construct_command_pools(Array<CommandPool>* pools, VulkanGpu* gpu, AllocatorBindings* bindings, VkAllocationCallbacks* callbacks);
    // void destroy_command_pools(Array<CommandPool>* pools, VulkanGpu* gpu, VkAllocationCallbacks* callbacks);
    // CommandPool* get_command_pool(Array<CommandPool>* pools, VkQueueFlags queueFlags);
    // //
    // // Command buffers
    // //
    // void construct_command_buffers(Array<VkCommandBuffer>* graphicsBufffers, VkCommandBuffer* transferBuffer, SwapChain* swapChain, VulkanGpu* gpu, Array<CommandPool>* pools, AllocatorBindings* bindings);
    // void destroy_command_buffers(Array<VkCommandBuffer>* graphicsBufffers, VkCommandBuffer* transferBuffer);

    // ==================================================================================================================
    //
    //
    // Backend interface
    //
    //
    // ==================================================================================================================

    void vulkan_backend_fill_vtable(RendererBackendVtable* table)
    {
        table->create = vulkan_backend_create;
        table->destroy = vulkan_backend_destroy;
        table->handle_resize = vulkan_backend_handle_resize;
        table->begin_frame = vulkan_backend_begin_frame;
        table->end_frame = vulkan_backend_end_frame;
        table->get_swap_chain_textures = vulkan_backend_get_swap_chain_textures;
        table->get_active_swap_chain_texture_index = vulkan_backend_get_active_swap_chain_texture_index;
        table->texture.create = vulkan_texture_create;
        table->texture.destroy = vulkan_texture_destroy;
        table->framebuffer.create = vulkan_framebuffer_create;
        table->framebuffer.destroy = vulkan_framebuffer_destroy;
        table->shaderProgram.create = vulkan_shader_program_create;
        table->shaderProgram.destroy = vulkan_shader_program_destroy;
        table->renderStage.create = vulkan_render_stage_create;
        table->renderStage.destroy = vulkan_render_stage_destroy;
        table->renderStage.bind = vulkan_render_stage_bind;
    }

    RendererBackend* vulkan_backend_create(RendererBackendCreateInfo* createInfo)
    {
        VulkanRendererBackend* backend = allocate<VulkanRendererBackend>(&createInfo->bindings);
        al_vk_memset(backend, 0, sizeof(VulkanRendererBackend));
        backend->window = createInfo->window;
        construct_memory_manager(&backend->memoryManager, &createInfo->bindings);

        data_block_storage_construct(&backend->shaderPrograms, &backend->memoryManager.cpu_allocationBindings);
        data_block_storage_construct(&backend->renderStages, &backend->memoryManager.cpu_allocationBindings);
        data_block_storage_construct(&backend->textures, &backend->memoryManager.cpu_allocationBindings);
        data_block_storage_construct(&backend->framebuffers, &backend->memoryManager.cpu_allocationBindings);

        construct_instance(&backend->instance, &backend->debugMessenger, createInfo, &backend->memoryManager.cpu_allocationBindings, &backend->memoryManager.cpu_allocationCallbacks);
        construct_surface(&backend->surface, backend->window, backend->instance, &backend->memoryManager.cpu_allocationCallbacks);
        construct_gpu(&backend->gpu, backend->instance, backend->surface, &backend->memoryManager.cpu_allocationBindings, &backend->memoryManager.cpu_allocationCallbacks);
        construct_swap_chain(&backend->swapChain, backend->surface, &backend->gpu, backend->window, &backend->memoryManager.cpu_allocationBindings, &backend->memoryManager.cpu_allocationCallbacks);
        construct_swap_chain_images(&backend->swapChain, &backend->gpu, &backend->memoryManager.cpu_allocationBindings, &backend->memoryManager.cpu_allocationCallbacks, &backend->textures);
        // construct_command_pools(&backend->commandPools, &backend->gpu, &backend->memoryManager.cpu_allocationBindings, &backend->memoryManager.cpu_allocationCallbacks);
        // construct_command_buffers(&backend->graphicsBuffers, &backend->transferBuffer, &backend->swapChain, &backend->gpu, &backend->commandPools, &backend->memoryManager.cpu_allocationBindings);
        {
            array_construct(&backend->commandPoolSets, &backend->memoryManager.cpu_allocationBindings, backend->swapChain.images.size);
            array_construct(&backend->commandBufferSets, &backend->memoryManager.cpu_allocationBindings, backend->swapChain.images.size);
            for (uSize it = 0; it < backend->swapChain.images.size; it++)
            {
                backend->commandPoolSets[it] = vulkan_command_pool_set_create(&backend->gpu, &backend->memoryManager);
                backend->commandBufferSets[it] = vulkan_command_buffer_set_create(&backend->commandPoolSets[it], &backend->gpu, &backend->memoryManager);
            }
        }
        {
            //
            // Create semaphores
            //
            VkSemaphoreCreateInfo semaphoreCreateInfo = utils::initializers::semaphore_create_info();
            for (auto it = create_iterator(&backend->imageAvailableSemaphores); !is_finished(&it); advance(&it))
            {
                al_vk_check(vkCreateSemaphore(backend->gpu.logicalHandle, &semaphoreCreateInfo, &backend->memoryManager.cpu_allocationCallbacks, get(it)));
            }
            for (auto it = create_iterator(&backend->renderFinishedSemaphores); !is_finished(&it); advance(&it))
            {
                al_vk_check(vkCreateSemaphore(backend->gpu.logicalHandle, &semaphoreCreateInfo, &backend->memoryManager.cpu_allocationCallbacks, get(it)));
            }
            //
            // Create fences
            //
            VkFenceCreateInfo fenceCreateInfo = utils::initializers::fence_create_info();
            for (auto it = create_iterator(&backend->inFlightFences); !is_finished(&it); advance(&it))
            {
                al_vk_check(vkCreateFence(backend->gpu.logicalHandle, &fenceCreateInfo, &backend->memoryManager.cpu_allocationCallbacks, get(it)));
            }
            array_construct(&backend->imageInFlightFencesRef, &backend->memoryManager.cpu_allocationBindings, backend->swapChain.images.size);
            for (auto it = create_iterator(&backend->imageInFlightFencesRef); !is_finished(&it); advance(&it))
            {
                al_vk_check(vkCreateFence(backend->gpu.logicalHandle, &fenceCreateInfo, &backend->memoryManager.cpu_allocationCallbacks, get(it)));
            }
        }
        return (RendererBackend*)backend;
    }

    void vulkan_backend_destroy(RendererBackend* _backend)
    {
        VulkanRendererBackend* backend = (VulkanRendererBackend*)_backend;
        {
            //
            // Destroy semaphores
            //
            for (auto it = create_iterator(&backend->imageAvailableSemaphores); !is_finished(&it); advance(&it))
            {
                vkDestroySemaphore(backend->gpu.logicalHandle, *get(it), &backend->memoryManager.cpu_allocationCallbacks);
            }
            for (auto it = create_iterator(&backend->renderFinishedSemaphores); !is_finished(&it); advance(&it))
            {
                vkDestroySemaphore(backend->gpu.logicalHandle, *get(it), &backend->memoryManager.cpu_allocationCallbacks);
            }
            //
            // Destroy fences
            //
            for (auto it = create_iterator(&backend->inFlightFences); !is_finished(&it); advance(&it))
            {
                vkDestroyFence(backend->gpu.logicalHandle, *get(it), &backend->memoryManager.cpu_allocationCallbacks);
            }
            for (auto it = create_iterator(&backend->imageInFlightFencesRef); !is_finished(&it); advance(&it))
            {
                vkDestroyFence(backend->gpu.logicalHandle, *get(it), &backend->memoryManager.cpu_allocationCallbacks);
            }
            array_destruct(&backend->imageInFlightFencesRef);
        }

        // destroy_command_buffers(&backend->graphicsBuffers, &backend->transferBuffer);
        // destroy_command_pools(&backend->commandPools, &backend->gpu, &backend->memoryManager.cpu_allocationCallbacks);
        destroy_swap_chain_images(&backend->swapChain, &backend->gpu, &backend->memoryManager);
        destroy_swap_chain(&backend->swapChain, &backend->gpu, &backend->memoryManager.cpu_allocationCallbacks);

        data_block_storage_destruct(&backend->shaderPrograms);
        data_block_storage_destruct(&backend->renderStages);
        data_block_storage_destruct(&backend->textures);
        data_block_storage_destruct(&backend->framebuffers);

        destroy_memory_manager(&backend->memoryManager, backend->gpu.logicalHandle);
        destroy_gpu(&backend->gpu, &backend->memoryManager.cpu_allocationCallbacks);
        destroy_surface(backend->surface, backend->instance, &backend->memoryManager.cpu_allocationCallbacks);
        destroy_instance(backend->instance, &backend->memoryManager.cpu_allocationCallbacks, backend->debugMessenger);
    }

    void vulkan_backend_handle_resize(RendererBackend* _backend)
    {
        VulkanRendererBackend* backend = (VulkanRendererBackend*)_backend;

        if (platform_window_is_minimized(backend->window))
        {
            return;
        }
        vkDeviceWaitIdle(backend->gpu.logicalHandle);
        {
            destroy_swap_chain(&backend->swapChain, &backend->gpu, &backend->memoryManager.cpu_allocationCallbacks);
        }
        {
            construct_swap_chain(&backend->swapChain, backend->surface, &backend->gpu, backend->window, &backend->memoryManager.cpu_allocationBindings, &backend->memoryManager.cpu_allocationCallbacks);
        }
    }

    void vulkan_backend_begin_frame(RendererBackend* _backend)
    {
        VulkanRendererBackend* backend = (VulkanRendererBackend*)_backend;
        backend->activeRenderFrame = utils::advance_render_frame(backend->activeRenderFrame);
        // Wait for current render frame processing to be finished
        vkWaitForFences(backend->gpu.logicalHandle, 1, &backend->inFlightFences[backend->activeRenderFrame], VK_TRUE, UINT64_MAX);
        // Get next swap chain image and set the semaphore to be signaled when image is free
        al_vk_check(vkAcquireNextImageKHR(backend->gpu.logicalHandle, backend->swapChain.handle, UINT64_MAX, backend->imageAvailableSemaphores[backend->activeRenderFrame], VK_NULL_HANDLE, &backend->activeSwapChainImageIndex));
        // If this swap chain image was used by some other render frame, we should wait for this render frame to be finished
        if (backend->imageInFlightFencesRef[backend->activeSwapChainImageIndex] != VK_NULL_HANDLE)
        {
            vkWaitForFences(backend->gpu.logicalHandle, 1, &backend->imageInFlightFencesRef[backend->activeSwapChainImageIndex], VK_TRUE, UINT64_MAX);
        }
        // Mark this image as used by this render frame
        backend->imageInFlightFencesRef[backend->activeSwapChainImageIndex] = backend->inFlightFences[backend->activeRenderFrame];
        VkCommandBuffer commandBuffer = vulkan_get_command_buffer(&backend->commandBufferSets[backend->activeSwapChainImageIndex], VK_QUEUE_GRAPHICS_BIT)->handle;
        VkCommandBufferBeginInfo beginInfo = utils::initializers::command_buffer_begin_info();
        al_vk_check(vkBeginCommandBuffer(commandBuffer, &beginInfo));
        backend->activeRenderStage = nullptr;
    }

    void vulkan_backend_end_frame(RendererBackend* _backend)
    {
        VulkanRendererBackend* backend = (VulkanRendererBackend*)_backend;
        VkCommandBuffer commandBuffer = vulkan_get_command_buffer(&backend->commandBufferSets[backend->activeSwapChainImageIndex], VK_QUEUE_GRAPHICS_BIT)->handle;
        vkCmdEndRenderPass(commandBuffer);
        al_vk_check(vkEndCommandBuffer(commandBuffer));
        // Queue gets processed only after swap chain image gets available
        VkSemaphore waitSemaphores[] =
        {
            backend->imageAvailableSemaphores[backend->activeRenderFrame]
        };
        VkPipelineStageFlags waitStages[] =
        {
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        };
        // @NOTE :  after finishing these semaphores will be signalled and image will be able to be presented on the screen
        VkSemaphore signalSemaphores[] =
        {
            backend->renderFinishedSemaphores[backend->activeRenderFrame]
        };
        VkSubmitInfo submitInfo
        {
            .sType                  = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext                  = nullptr,
            .waitSemaphoreCount     = array_size(waitSemaphores),
            .pWaitSemaphores        = waitSemaphores,
            .pWaitDstStageMask      = waitStages,
            .commandBufferCount     = 1,
            .pCommandBuffers        = &commandBuffer,
            .signalSemaphoreCount   = array_size(signalSemaphores),
            .pSignalSemaphores      = signalSemaphores,
        };
        vkResetFences(backend->gpu.logicalHandle, 1, &backend->inFlightFences[backend->activeRenderFrame]);
        vkQueueSubmit(get_command_queue(&backend->gpu, VulkanGpu::CommandQueue::GRAPHICS)->handle, 1, &submitInfo, backend->inFlightFences[backend->activeRenderFrame]);
        VkSwapchainKHR swapChains[] =
        {
            backend->swapChain.handle,
        };
        VkPresentInfoKHR presentInfo
        {
            .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pNext              = nullptr,
            .waitSemaphoreCount = array_size(signalSemaphores),
            .pWaitSemaphores    = signalSemaphores,
            .swapchainCount     = array_size(swapChains),
            .pSwapchains        = swapChains,
            .pImageIndices      = &backend->activeSwapChainImageIndex,
            .pResults           = nullptr,
        };
        al_vk_check(vkQueuePresentKHR(get_command_queue(&backend->gpu, VulkanGpu::CommandQueue::PRESENT)->handle, &presentInfo));
        // @NOTE :  if VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT flag in VkCommandPoolCreateInfo is true,
        //          command buffer gets implicitly reset when calling vkBeginCommandBuffer
        //          https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkCommandPoolCreateFlagBits.html
        // al_vk_check(vkResetCommandBuffer(commandBuffer, 0));
    }

    PointerWithSize<Texture*> vulkan_backend_get_swap_chain_textures(RendererBackend* _backend)
    {
        VulkanRendererBackend* backend = (VulkanRendererBackend*)_backend;
        return { .ptr = backend->swapChain.images.memory, .size = backend->swapChain.images.size, };
    }

    uSize vulkan_backend_get_active_swap_chain_texture_index(RendererBackend* _backend)
    {
        VulkanRendererBackend* backend = (VulkanRendererBackend*)_backend;
        return backend->activeSwapChainImageIndex;
    }

    // ==================================================================================================================
    //
    //
    // Memory manager
    //
    //
    // ==================================================================================================================

    static VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_callback(
                                            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                            VkDebugUtilsMessageTypeFlagsEXT messageType,
                                            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                            void* pUserData) 
    {
        printf("Debug callback : %s\n", pCallbackData->pMessage);
        return VK_FALSE;
    }

    // ==================================================================================================================
    //
    //
    // Instance
    //
    //
    // ==================================================================================================================

    static void construct_instance(VkInstance* instance, VkDebugUtilsMessengerEXT* debugMessenger, RendererBackendCreateInfo* backendCreateInfo, AllocatorBindings* bindings, VkAllocationCallbacks* callbacks)
    {
        const bool isDebug = backendCreateInfo->flags & RendererBackendCreateInfo::IS_DEBUG;
        if (isDebug)
        {
            Array<VkLayerProperties> availableValidationLayers = utils::get_available_validation_layers(bindings);
            defer(array_destruct(&availableValidationLayers));
            for (uSize requiredIt = 0; requiredIt < array_size(utils::VALIDATION_LAYERS); requiredIt++)
            {
                const char* requiredLayerName = utils::VALIDATION_LAYERS[requiredIt];
                bool isFound = false;
                for (auto availableIt = create_iterator(&availableValidationLayers); !is_finished(&availableIt); advance(&availableIt))
                {
                    const char* availableLayerName = get(availableIt)->layerName;
                    if (al_vk_strcmp(availableLayerName, requiredLayerName))
                    {
                        isFound = true;
                        break;
                    }
                }
                if (!isFound)
                {
                    al_vk_assert(!"Required validation layer is not supported");
                    // @TODO :  handle this somehow
                }
            }
        }
        {
            Array<VkExtensionProperties> availableInstanceExtensions = utils::get_available_instance_extensions(bindings);
            defer(array_destruct(&availableInstanceExtensions));
            for (uSize requiredIt = 0; requiredIt < array_size(utils::INSTANCE_EXTENSIONS); requiredIt++)
            {
                const char* requiredInstanceExtensionName = utils::INSTANCE_EXTENSIONS[requiredIt];
                bool isFound = false;
                for (auto availableIt = create_iterator(&availableInstanceExtensions); !is_finished(&availableIt); advance(&availableIt))
                {
                    const char* availableExtensionName = get(availableIt)->extensionName;
                    if (al_vk_strcmp(availableExtensionName, requiredInstanceExtensionName))
                    {
                        isFound = true;
                        break;
                    }
                }
                if (!isFound)
                {
                    al_vk_assert(!"Required instance extension is not supported");
                    // @TODO :  handle this somehow
                }
            }
        }
        VkApplicationInfo applicationInfo
        {
            .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pNext              = nullptr,
            .pApplicationName   = backendCreateInfo->applicationName,
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName        = EngineConfig::ENGINE_NAME,
            .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion         = VK_API_VERSION_1_0
        };
        // @NOTE :  although we setuping debug messenger in the end of this function,
        //          created debug messenger will not be able to recieve information about
        //          vkCreateInstance and vkDestroyInstance calls. So we have to pass additional
        //          VkDebugUtilsMessengerCreateInfoEXT struct pointer to VkInstanceCreateInfo::pNext
        //          to enable debug of these functions
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = utils::get_debug_messenger_create_info(vulkan_debug_callback);
        VkInstanceCreateInfo instanceCreateInfo
        {
            .sType                      = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext                      = isDebug ? &debugCreateInfo : nullptr,
            .flags                      = 0,
            .pApplicationInfo           = &applicationInfo,
            .enabledLayerCount          = isDebug ? array_size(utils::VALIDATION_LAYERS) : 0,
            .ppEnabledLayerNames        = isDebug ? utils::VALIDATION_LAYERS : nullptr,
            .enabledExtensionCount      = array_size(utils::INSTANCE_EXTENSIONS),
            .ppEnabledExtensionNames    = utils::INSTANCE_EXTENSIONS,
        };
        al_vk_check(vkCreateInstance(&instanceCreateInfo, callbacks, instance));
        if (isDebug)
        {
            *debugMessenger = utils::create_debug_messenger(&debugCreateInfo, *instance, callbacks);
        }
    }

    void destroy_instance(VkInstance instance, VkAllocationCallbacks* callbacks, VkDebugUtilsMessengerEXT messenger)
    {
        if (messenger != VK_NULL_HANDLE)
        {
            utils::destroy_debug_messenger(instance, messenger, callbacks);
        }
        vkDestroyInstance(instance, callbacks);
    }

    // ==================================================================================================================
    //
    //
    // Surface
    //
    //
    // ==================================================================================================================

    void construct_surface(VkSurfaceKHR* surface, PlatformWindow* window, VkInstance instance, VkAllocationCallbacks* callbacks)
    {
#ifdef _WIN32
        {
            VkWin32SurfaceCreateInfoKHR surfaceCreateInfo
            {
                .sType      = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
                .pNext      = nullptr,
                .flags      = 0,
                .hinstance  = ::GetModuleHandle(nullptr),
                .hwnd       = window->handle
            };
            al_vk_check(vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, callbacks, surface));
        }
#else
#   error Unsupported platform
#endif
    }

    void destroy_surface(VkSurfaceKHR surface, VkInstance instance, VkAllocationCallbacks* callbacks)
    {
        vkDestroySurfaceKHR(instance, surface, callbacks);
    }

    // ==================================================================================================================
    //
    //
    // Swap chain
    //
    //
    // ==================================================================================================================

    void fill_swap_chain_sharing_mode(VkSharingMode* resultMode, u32* resultCount, PointerWithSize<u32> resultFamilyIndices, VulkanGpu* gpu)
    {
        VulkanGpu::CommandQueue* graphicsQueue = get_command_queue(gpu, VulkanGpu::CommandQueue::GRAPHICS);
        VulkanGpu::CommandQueue* presentQueue = get_command_queue(gpu, VulkanGpu::CommandQueue::PRESENT);
        if (graphicsQueue->queueFamilyIndex == presentQueue->queueFamilyIndex)
        {
            *resultMode = VK_SHARING_MODE_EXCLUSIVE;
            *resultCount = 0;
        }
        else
        {
            *resultMode = VK_SHARING_MODE_CONCURRENT;
            *resultCount = 2;
            resultFamilyIndices[0] = graphicsQueue->queueFamilyIndex;
            resultFamilyIndices[1] = presentQueue->queueFamilyIndex;
        }
    }

    void construct_swap_chain(SwapChain* swapChain, VkSurfaceKHR surface, VulkanGpu* gpu, PlatformWindow* window, AllocatorBindings* bindings, VkAllocationCallbacks* callbacks)
    {
        utils::SwapChainSupportDetails supportDetails = utils::create_swap_chain_support_details(surface, gpu->physicalHandle, bindings);
        defer(utils::destroy_swap_chain_support_details(&supportDetails));
        VkSurfaceFormatKHR  surfaceFormat   = utils::choose_swap_chain_surface_format(supportDetails.formats);
        VkPresentModeKHR    presentMode     = utils::choose_swap_chain_surface_present_mode(supportDetails.presentModes);
        VkExtent2D          extent          = utils::choose_swap_chain_extent(platform_window_get_current_width(window), platform_window_get_current_width(window), &supportDetails.capabilities);
        // @NOTE :  supportDetails.capabilities.maxImageCount == 0 means unlimited amount of images in swapchain
        u32 imageCount = supportDetails.capabilities.minImageCount + 1;
        if (supportDetails.capabilities.maxImageCount != 0 && imageCount > supportDetails.capabilities.maxImageCount)
        {
            imageCount = supportDetails.capabilities.maxImageCount;
        }
        StaticPointerWithSize<u32, VulkanGpu::MAX_UNIQUE_COMMAND_QUEUES> queueFamiliesIndices = {};
        VkSwapchainCreateInfoKHR createInfo
        {
            .sType                  = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .pNext                  = nullptr,
            .flags                  = 0,
            .surface                = surface,
            .minImageCount          = imageCount,
            .imageFormat            = surfaceFormat.format,
            .imageColorSpace        = surfaceFormat.colorSpace,
            .imageExtent            = extent,
            .imageArrayLayers       = 1,
            .imageUsage             = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageSharingMode       = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount  = 0,
            .pQueueFamilyIndices    = queueFamiliesIndices.ptr,
            .preTransform           = supportDetails.capabilities.currentTransform, // Allows to apply transform to images (rotate etc)
            .compositeAlpha         = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode            = presentMode,
            .clipped                = VK_TRUE,
            .oldSwapchain           = VK_NULL_HANDLE,
        };
        fill_swap_chain_sharing_mode(&createInfo.imageSharingMode, &createInfo.queueFamilyIndexCount, { .ptr = queueFamiliesIndices.ptr, .size = queueFamiliesIndices.size }, gpu);
        al_vk_check(vkCreateSwapchainKHR(gpu->logicalHandle, &createInfo, callbacks, &swapChain->handle));
        swapChain->surfaceFormat = surfaceFormat;
        swapChain->extent = extent;
    }

    void destroy_swap_chain(SwapChain* swapChain, VulkanGpu* gpu, VkAllocationCallbacks* callbacks)
    {
        vkDestroySwapchainKHR(gpu->logicalHandle, swapChain->handle, callbacks);
    }

    void construct_swap_chain_images(SwapChain* swapChain, VulkanGpu* gpu, AllocatorBindings* bindings, VkAllocationCallbacks* callbacks, VulkanRendererBackend::TextureStorage* textureStorage)
    {
        u32 swapChainImageCount;
        al_vk_check(vkGetSwapchainImagesKHR(gpu->logicalHandle, swapChain->handle, &swapChainImageCount, nullptr));
        array_construct(&swapChain->images, bindings, swapChainImageCount);
        Array<VkImage> swapChainImages;
        array_construct(&swapChainImages, bindings, swapChainImageCount);
        defer(array_destruct(&swapChainImages));
        // It seems like this vkGetSwapchainImagesKHR call leaks memory
        al_vk_check(vkGetSwapchainImagesKHR(gpu->logicalHandle, swapChain->handle, &swapChainImageCount, swapChainImages.memory));
        for (u32 it = 0; it < swapChainImageCount; it++)
        {
            VkImageView view = VK_NULL_HANDLE;
            VkImageViewCreateInfo createInfo
            {
                .sType              = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .pNext              = nullptr,
                .flags              = 0,
                .image              = swapChainImages[it],
                .viewType           = VK_IMAGE_VIEW_TYPE_2D,
                .format             = swapChain->surfaceFormat.format,
                .components         = 
                {
                    .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .a = VK_COMPONENT_SWIZZLE_IDENTITY
                },
                .subresourceRange   = 
                {
                    .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel   = 0,
                    .levelCount     = 1,
                    .baseArrayLayer = 0,
                    .layerCount     = 1
                }
            };
            al_vk_check(vkCreateImageView(gpu->logicalHandle, &createInfo, callbacks, &view));
            VulkanTexture* texture = data_block_storage_add(textureStorage);
            vulkan_swap_chain_texture_construct(texture, view, swapChain->extent, swapChain->surfaceFormat.format, gpu->depthStencilFormat);
            swapChain->images[it] = texture;
        }
    }

    void destroy_swap_chain_images(SwapChain* swapChain, VulkanGpu* gpu, VulkanMemoryManager* memoryManager)
    {
        for (auto it = create_iterator(&swapChain->images); !is_finished(&it); advance(&it))
        {
            vulkan_texture_destroy_internal(gpu, memoryManager, (VulkanTexture*)*get(it));
        }
        array_destruct(&swapChain->images);
    }

    // // ==================================================================================================================
    // //
    // //
    // // Command pools
    // //
    // //
    // // ==================================================================================================================

    // void construct_command_pools(Array<CommandPool>* pools, VulkanGpu* gpu, AllocatorBindings* bindings, VkAllocationCallbacks* callbacks)
    // {
    //     constexpr VkCommandPoolCreateFlags POOL_FLAGS = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    //     VulkanGpu::CommandQueue* graphicsQueue = get_command_queue(gpu, VulkanGpu::CommandQueue::GRAPHICS);
    //     VulkanGpu::CommandQueue* transferQueue = get_command_queue(gpu, VulkanGpu::CommandQueue::TRANSFER);
    //     if (graphicsQueue->queueFamilyIndex == transferQueue->queueFamilyIndex)
    //     {
    //         array_construct(pools, bindings, 1);
    //         (*pools)[0] = 
    //         {
    //             .handle = utils::create_command_pool(gpu->logicalHandle, graphicsQueue->queueFamilyIndex, callbacks, POOL_FLAGS),
    //             .queueFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT,
    //         };
    //     }
    //     else
    //     {
    //         array_construct(pools, bindings, 2);
    //         (*pools)[0] =
    //         {
    //             .handle = utils::create_command_pool(gpu->logicalHandle, graphicsQueue->queueFamilyIndex, callbacks, POOL_FLAGS),
    //             .queueFlags = VK_QUEUE_GRAPHICS_BIT,
    //         };
    //         (*pools)[1] =
    //         {
    //             .handle = utils::create_command_pool(gpu->logicalHandle, transferQueue->queueFamilyIndex, callbacks, POOL_FLAGS),
    //             .queueFlags = VK_QUEUE_TRANSFER_BIT,
    //         };
    //     }
    // }

    // void destroy_command_pools(Array<CommandPool>* pools, VulkanGpu* gpu, VkAllocationCallbacks* callbacks)
    // {
    //     for (auto it = create_iterator(pools); !is_finished(&it); advance(&it))
    //     {
    //         utils::destroy_command_pool(get(it)->handle, gpu->logicalHandle, callbacks);
    //     }
    //     array_destruct(pools);
    // }

    // CommandPool* get_command_pool(Array<CommandPool>* pools, VkQueueFlags queueFlags)
    // {
    //     for (auto it = create_iterator(pools); !is_finished(&it); advance(&it))
    //     {
    //         CommandPool* pool = get(it);
    //         if (pool->queueFlags & queueFlags)
    //         {
    //             return pool;
    //         }
    //     }
    //     return nullptr;
    // }

    // // ==================================================================================================================
    // //
    // //
    // // Command buffers
    // //
    // //
    // // ==================================================================================================================

    // void construct_command_buffers(Array<VkCommandBuffer>* graphicsBufffers, VkCommandBuffer* transferBuffer, SwapChain* swapChain, VulkanGpu* gpu, Array<CommandPool>* pools, AllocatorBindings* bindings)
    // {
    //     array_construct(graphicsBufffers, bindings, swapChain->images.size);
    //     VkCommandPool graphicsPool = get_command_pool(pools, VK_QUEUE_GRAPHICS_BIT)->handle;
    //     for (auto it = create_iterator(graphicsBufffers); !is_finished(&it); advance(&it))
    //     {
    //         // @TODO :  create command buffers for each possible thread
    //         *get(it) = utils::create_command_buffer(gpu->logicalHandle, graphicsPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    //     }
    //     *transferBuffer = utils::create_command_buffer(gpu->logicalHandle, get_command_pool(pools, VK_QUEUE_TRANSFER_BIT)->handle, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    // }

    // void destroy_command_buffers(Array<VkCommandBuffer>* graphicsBufffers, VkCommandBuffer* transferBuffer)
    // {
    //     array_destruct(graphicsBufffers);
    // }
}

#include "vulkan_utils.cpp"
#include "vulkan_utils_initializers.cpp"
#include "vulkan_utils_converters.cpp"
#include "vulkan_memory_manager.cpp"
#include "vulkan_gpu.cpp"
#include "vulkan_command_buffers.cpp"
#include "vulkan_memory_buffer.cpp"

#include "texture_vulkan.cpp"
#include "framebuffer_vulkan.cpp"
#include "render_stage_vulkan.cpp"
