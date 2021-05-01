
#include <cassert>
#include <cstdlib>
#include <cstring>

#include "renderer_backend_vulkan.h"

#include "engine/config.h"
#include "engine/utilities/utilities.h"

#define al_vk_log_msg(format, ...)      std::fprintf(stdout, format, __VA_ARGS__);
#define al_vk_log_error(format, ...)    std::fprintf(stderr, format, __VA_ARGS__);

#define USE_DYNAMIC_STATE

namespace al
{
    template<>
    void renderer_backend_construct<vulkan::RendererBackend>(vulkan::RendererBackend* backend, RendererBackendInitData* initData)
    {
        using namespace vulkan;
        backend->window = initData->window;
        construct                       (&backend->memoryManager, initData->bindings);
        create_instance                 (backend, initData);
        create_surface                  (backend);
        pick_gpu_and_init_command_queues(backend);
        create_swap_chain               (backend);
        create_depth_stencil            (backend);
        create_render_passes            (backend);
        create_render_pipelines         (backend);
        create_framebuffers             (backend);
        create_sync_primitives          (backend);

        // @NOTE :  Do we really need a thread-local storage for command pools and command buffers? need to think about this
        // @TODO :  create command pools and buffers for each thread in advance, so we won't have to do it in the middle of running (in get_command_pools/buffers functions)
        // tls_construct(&backend->_commandPools, initData->bindings);
        // tls_construct(&backend->_commandBuffers, initData->bindings);
        create_command_pools(backend, &backend->_commandPools);
        create_command_buffers(backend, &backend->_commandBuffers);

        // Filling triangle buffer
        backend->_vertexBuffer = create_vertex_buffer(backend, sizeof(triangle));
        backend->_indexBuffer = create_index_buffer(backend, sizeof(triangleIndices));
        // Staging buffer occupies the whole memory chunk
        backend->_stagingBuffer = create_staging_buffer(backend, VulkanMemoryManager::GPU_CHUNK_SIZE_BYTES);
        copy_cpu_memory_to_buffer(backend, triangle, &backend->_stagingBuffer, sizeof(triangle));
        copy_buffer_to_buffer(backend, &backend->_stagingBuffer, &backend->_vertexBuffer, sizeof(triangle));
        copy_cpu_memory_to_buffer(backend, triangleIndices, &backend->_stagingBuffer, sizeof(triangleIndices));
        copy_buffer_to_buffer(backend, &backend->_stagingBuffer, &backend->_indexBuffer, sizeof(triangleIndices));
    }

    template<>
    void renderer_backend_render<vulkan::RendererBackend>(vulkan::RendererBackend* backend)
    {
        using namespace vulkan;
        backend->currentRenderFrame = vk::advance_render_frame(backend->currentRenderFrame);
        // @NOTE :  wait for current render frame processing to be finished
        vkWaitForFences(backend->gpu.logicalHandle, 1, &backend->syncPrimitives.inFlightFences[backend->currentRenderFrame], VK_TRUE, UINT64_MAX);
        // @NOTE :  get next swap chain image and set the semaphore to be signaled when image is free
        u32 swapChainImageIndex;
        al_vk_check(vkAcquireNextImageKHR(backend->gpu.logicalHandle, backend->swapChain.handle, UINT64_MAX, backend->syncPrimitives.imageAvailableSemaphores[backend->currentRenderFrame], VK_NULL_HANDLE, &swapChainImageIndex));
        // @NOTE :  if this swap chain image was used by some other render frame, we should wait for this render frame to be finished
        if (backend->syncPrimitives.imageInFlightFencesRef[swapChainImageIndex] != VK_NULL_HANDLE)
        {
            vkWaitForFences(backend->gpu.logicalHandle, 1, &backend->syncPrimitives.imageInFlightFencesRef[swapChainImageIndex], VK_TRUE, UINT64_MAX);
        }
        // @NOTE :  mark this image as used by this render frame
        backend->syncPrimitives.imageInFlightFencesRef[swapChainImageIndex] = backend->syncPrimitives.inFlightFences[backend->currentRenderFrame];
        VkCommandBuffer commandBuffer = get_command_buffers(backend)->primaryBuffers[swapChainImageIndex];
        VkCommandBufferBeginInfo beginInfo
        {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext              = nullptr,
            .flags              = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            .pInheritanceInfo   = nullptr,
        };
        al_vk_check(vkBeginCommandBuffer(commandBuffer, &beginInfo));
        {
            {   // @NOTE :  set dynamic state values
                VkViewport viewport
                {
                    .x          = 0.0f,
                    .y          = 0.0f,
                    .width      = static_cast<float>(backend->swapChain.extent .width),
                    .height     = static_cast<float>(backend->swapChain.extent .height),
                    .minDepth   = 0.0f,
                    .maxDepth   = 1.0f,
                };
                VkRect2D scissor
                {
                    .offset = { 0, 0 },
                    .extent = backend->swapChain.extent ,
                };
                vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
                vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
            }
            {   // @NOTE :  render pass
                VkClearValue clearValues[] =
                {
                    { .color = { 1.0f, 0.4f, 0.1f, 1.0f } },
                    { .depthStencil = { 1.0f, 0 } }
                };
                VkRenderPassBeginInfo renderPassInfo
                {
                    .sType              = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                    .pNext              = nullptr,
                    .renderPass         = backend->simpleRenderPass,
                    .framebuffer        = backend->swapChain.framebuffers[swapChainImageIndex],
                    .renderArea         = { .offset = { 0, 0 }, .extent = backend->swapChain.extent },
                    .clearValueCount    = array_size(clearValues),
                    .pClearValues       = clearValues,
                };
                vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, backend->pipeline);
                VkBuffer vertexBuffers[] = { backend->_vertexBuffer.handle };
                VkDeviceSize offsets[] = { 0 };
                vkCmdBindVertexBuffers(commandBuffer, 0, array_size(vertexBuffers), vertexBuffers, offsets);
                vkCmdBindIndexBuffer(commandBuffer, backend->_indexBuffer.handle, 0, VK_INDEX_TYPE_UINT32);
                vkCmdDrawIndexed(commandBuffer, array_size(triangleIndices), 1, 0, 0, 0);
                vkCmdEndRenderPass(commandBuffer);
            }
        }
        al_vk_check(vkEndCommandBuffer(commandBuffer));
        {   // @NOTE :  submit buffer and present image
            // @NOTE :  queue gets processed only after swap chain image gets available
            VkSemaphore waitSemaphores[] =
            {
                backend->syncPrimitives.imageAvailableSemaphores[backend->currentRenderFrame]
            };
            VkPipelineStageFlags waitStages[] =
            {
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
            };
            // @NOTE :  after finishing these semaphores will be signalled and image will be able to be presented on the screen
            VkSemaphore signalSemaphores[] =
            {
                backend->syncPrimitives.renderFinishedSemaphores[backend->currentRenderFrame]
            };
            VkSubmitInfo submitInfo
            {
                .sType                  = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                .pNext                  = nullptr,
                .waitSemaphoreCount     = sizeof(waitSemaphores) / sizeof(waitSemaphores[0]),
                .pWaitSemaphores        = waitSemaphores,
                .pWaitDstStageMask      = waitStages,
                .commandBufferCount     = 1,
                .pCommandBuffers        = &commandBuffer,
                .signalSemaphoreCount   = sizeof(signalSemaphores) / sizeof(signalSemaphores[0]),
                .pSignalSemaphores      = signalSemaphores,
            };
            vkResetFences(backend->gpu.logicalHandle, 1, &backend->syncPrimitives.inFlightFences[backend->currentRenderFrame]);
            vkQueueSubmit(backend->queues.graphics.handle, 1, &submitInfo, backend->syncPrimitives.inFlightFences[backend->currentRenderFrame]);
            VkSwapchainKHR swapChains[] =
            {
                backend->swapChain.handle
            };
            VkPresentInfoKHR presentInfo
            {
                .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                .pNext              = nullptr,
                .waitSemaphoreCount = sizeof(signalSemaphores) / sizeof(signalSemaphores[0]),
                .pWaitSemaphores    = signalSemaphores,
                .swapchainCount     = sizeof(swapChains) / sizeof(swapChains[0]),
                .pSwapchains        = swapChains,
                .pImageIndices      = &swapChainImageIndex,
                .pResults           = nullptr,
            };
            al_vk_check(vkQueuePresentKHR(backend->queues.present.handle, &presentInfo));
            // @NOTE :  if VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT flag in VkCommandPoolCreateInfo is true,
            //          command buffer gets implicitly reset when calling vkBeginCommandBuffer
            //          https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkCommandPoolCreateFlagBits.html
            // al_vk_check(vkResetCommandBuffer(commandBuffer, 0));
        }
    }

    template<>
    void renderer_backend_destruct<vulkan::RendererBackend>(vulkan::RendererBackend* backend)
    {
        using namespace vulkan;
        vkDeviceWaitIdle(backend->gpu.logicalHandle);

        destroy_buffer(backend, &backend->_indexBuffer);
        destroy_buffer(backend, &backend->_vertexBuffer);
        destroy_buffer(backend, &backend->_stagingBuffer);

        // for (uSize it = 0; it < backend->_commandBuffers.size; it++)
        // {
        //     destroy_command_buffers(backend, &backend->_commandBuffers.memory[it]);
        // }
        // tls_destruct(&backend->_commandBuffers);
        // for (uSize it = 0; it < backend->_commandPools.size; it++)
        // {
        //     destroy_command_pools(backend, &backend->_commandPools.memory[it]);
        // }
        // tls_destruct(&backend->_commandPools);
        destroy_command_buffers(backend, &backend->_commandBuffers);
        destroy_command_pools(backend, &backend->_commandPools);

        destroy_sync_primitives     (backend);
        destroy_framebuffers        (backend);
        destroy_render_pipelines    (backend);
        destroy_render_passes       (backend);
        destroy_depth_stencil       (backend);
        destroy_swap_chain          (backend);
        destruct                    (&backend->memoryManager, backend->gpu.logicalHandle);
        destroy_gpu                 (backend);
        destroy_surface             (backend);
        destroy_instance            (backend);
    }

    template<>
    void renderer_backend_handle_resize<vulkan::RendererBackend>(vulkan::RendererBackend* backend)
    {
        using namespace vulkan;
        if (platform_window_is_minimized(backend->window))
        {
            return;
        }
        vkDeviceWaitIdle(backend->gpu.logicalHandle);
        {
            destroy_framebuffers    (backend);
            destroy_render_pipelines(backend);
            destroy_render_passes   (backend);
            destroy_depth_stencil   (backend);
            destroy_swap_chain      (backend);
        }
        {
            create_swap_chain       (backend);
            create_depth_stencil    (backend);
            create_render_passes    (backend);
            create_render_pipelines (backend);
            create_framebuffers     (backend);
        }
    }
}

namespace al::vulkan
{
    // =================================================================================================================
    // CREATION
    // =================================================================================================================

    void create_instance(RendererBackend* backend, RendererBackendInitData* initData)
    {
#ifdef AL_DEBUG
        {
            ArrayView<VkLayerProperties> availableValidationLayers = vk::get_available_validation_layers(&backend->memoryManager.cpu_allocationBindings);
            defer(av_destruct(availableValidationLayers));
            for (uSize requiredIt = 0; requiredIt < array_size(vk::VALIDATION_LAYERS); requiredIt++)
            {
                const char* requiredLayerName = vk::VALIDATION_LAYERS[requiredIt];
                bool isFound = false;
                for (uSize availableIt = 0; availableIt < availableValidationLayers.count; availableIt++)
                {
                    const char* availableLayerName = availableValidationLayers[availableIt].layerName;
                    if (al_vk_strcmp(availableLayerName, requiredLayerName))
                    {
                        isFound = true;
                        break;
                    }
                }
                if (!isFound)
                {
                    al_vk_log_error("Required validation layer \"%s\" is not supported\n", requiredLayerName);
                    // @TODO :  handle this somehow
                }
            }
        }
#endif
        {
            ArrayView<VkExtensionProperties> availableInstanceExtensions = vk::get_available_instance_extensions(&backend->memoryManager.cpu_allocationBindings);
            defer(av_destruct(availableInstanceExtensions));
            for (uSize requiredIt = 0; requiredIt < array_size(vk::INSTANCE_EXTENSIONS); requiredIt++)
            {
                const char* requiredInstanceExtensionName = vk::INSTANCE_EXTENSIONS[requiredIt];
                bool isFound = false;
                for (uSize availableIt = 0; availableIt < availableInstanceExtensions.count; availableIt++)
                {
                    const char* availableExtensionName = availableInstanceExtensions[availableIt].extensionName;
                    if (al_vk_strcmp(availableExtensionName, requiredInstanceExtensionName))
                    {
                        isFound = true;
                        break;
                    }
                }
                if (!isFound)
                {
                    al_vk_log_error("Required instance extension \"%s\" is not supported\n", requiredInstanceExtensionName);
                    // @TODO :  handle this somehow
                }
            }
        }
        VkApplicationInfo applicationInfo
        {
            .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pNext              = nullptr,
            .pApplicationName   = initData->applicationName,
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
        VkInstanceCreateInfo instanceCreateInfo{ };
        instanceCreateInfo.sType                    = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceCreateInfo.pApplicationInfo         = &applicationInfo;
        instanceCreateInfo.enabledExtensionCount    = array_size(vk::INSTANCE_EXTENSIONS);
        instanceCreateInfo.ppEnabledExtensionNames  = vk::INSTANCE_EXTENSIONS;
#ifdef AL_DEBUG
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = vk::get_debug_messenger_create_info(vulkan_debug_callback);
        instanceCreateInfo.pNext                = &debugCreateInfo;
        instanceCreateInfo.enabledLayerCount    = array_size(vk::VALIDATION_LAYERS);
        instanceCreateInfo.ppEnabledLayerNames  = vk::VALIDATION_LAYERS;
#endif
        al_vk_check(vkCreateInstance(&instanceCreateInfo, &backend->memoryManager.cpu_allocationCallbacks, &backend->instance));
#ifdef AL_DEBUG
        backend->debugMessenger = vk::create_debug_messenger(&debugCreateInfo, backend->instance, &backend->memoryManager.cpu_allocationCallbacks);
#endif
    }

    void create_surface(RendererBackend* backend)
    {
#ifdef _WIN32
        {
            VkWin32SurfaceCreateInfoKHR surfaceCreateInfo
            {
                .sType      = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
                .pNext      = nullptr,
                .flags      = 0,
                .hinstance  = ::GetModuleHandle(nullptr),
                .hwnd       = backend->window->handle
            };
            al_vk_check(vkCreateWin32SurfaceKHR(backend->instance, &surfaceCreateInfo, &backend->memoryManager.cpu_allocationCallbacks, &backend->surface));
        }
#else
#   error Unsupported platform
#endif
    }

    void pick_gpu_and_init_command_queues(RendererBackend* backend)
    {
        auto getQueueCreateInfos = [](RendererBackend* backend, CommandQueues* queues) -> ArrayView<VkDeviceQueueCreateInfo>
        {
            // @NOTE :  this is possible that queue family might support more than one of the required features,
            //          so we have to remove duplicates from queueFamiliesInfo and create VkDeviceQueueCreateInfos
            //          only for unique indexes
            static const f32 QUEUE_DEFAULT_PRIORITY = 1.0f;
            u32 indicesArray[CommandQueues::QUEUES_NUM];
            ArrayView<u32> uniqueQueueIndices
            {
                .memory = indicesArray,
                .count = 0
            };
            auto updateUniqueIndicesArray = [](ArrayView<u32>* targetArray, u32 familyIndex)
            {
                bool isFound = false;
                for (uSize uniqueIt = 0; uniqueIt < targetArray->count; uniqueIt++)
                {
                    if ((*targetArray)[uniqueIt] == familyIndex)
                    {
                        isFound = true;
                        break;
                    }
                }
                if (!isFound)
                {
                    targetArray->count += 1;
                    (*targetArray)[targetArray->count - 1] = familyIndex;
                }
            };
            for (uSize it = 0; it < CommandQueues::QUEUES_NUM; it++)
            {
                updateUniqueIndicesArray(&uniqueQueueIndices, queues->queues[it].deviceFamilyIndex);
            }
            ArrayView<VkDeviceQueueCreateInfo> result;
            av_construct(&result, &backend->memoryManager.cpu_allocationBindings, uniqueQueueIndices.count);
            for (uSize it = 0; it < result.count; it++)
            {
                result[it] = 
                {
                    .sType              = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                    .pNext              = nullptr,
                    .flags              = 0,
                    .queueFamilyIndex   = uniqueQueueIndices[it],
                    .queueCount         = 1,
                    .pQueuePriorities   = &QUEUE_DEFAULT_PRIORITY,
                };
            }
            return result;
        };
        auto[commandQueues, physicalDevice] = pick_physical_device(backend);
        al_vk_assert(physicalDevice != VK_NULL_HANDLE);
        VkDevice logicalDevice = VK_NULL_HANDLE;
        ArrayView<VkDeviceQueueCreateInfo> queueCreateInfos = getQueueCreateInfos(backend, &commandQueues);
        defer(av_destruct(queueCreateInfos));
        VkPhysicalDeviceFeatures deviceFeatures{ };
        VkDeviceCreateInfo logicalDeviceCreateInfo
        {
            .sType                      = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext                      = nullptr,
            .flags                      = 0,
            .queueCreateInfoCount       = static_cast<u32>(queueCreateInfos.count),
            .pQueueCreateInfos          = queueCreateInfos.memory,
            // @NOTE : Validation layers info is not used by recent vulkan implemetations, but still can be set for compatibility reasons
            .enabledLayerCount          = array_size(vk::VALIDATION_LAYERS),
            .ppEnabledLayerNames        = vk::VALIDATION_LAYERS,
            .enabledExtensionCount      = array_size(vk::DEVICE_EXTENSIONS),
            .ppEnabledExtensionNames    = vk::DEVICE_EXTENSIONS,
            .pEnabledFeatures           = &deviceFeatures
        };
        al_vk_check(vkCreateDevice(physicalDevice, &logicalDeviceCreateInfo, &backend->memoryManager.cpu_allocationCallbacks, &logicalDevice));
        for (uSize it = 0; it < CommandQueues::QUEUES_NUM; it++)
        {
            vkGetDeviceQueue(logicalDevice, commandQueues.queues[it].deviceFamilyIndex, 0, &commandQueues.queues[it].handle);
        }
        vk::pick_depth_format(physicalDevice, &backend->depthStencil.format);
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &backend->gpu.memoryProperties);
        backend->gpu.physicalHandle = physicalDevice;
        backend->gpu.logicalHandle = logicalDevice;
        std::memcpy(&backend->queues, &commandQueues, sizeof(CommandQueues));
    }

    void create_swap_chain(RendererBackend* backend)
    {
        auto chooseSurfaceFormat = [](ArrayView<VkSurfaceFormatKHR> formats) -> VkSurfaceFormatKHR
        {
            for (uSize it = 0; it < formats.count; it++)
            {
                if (formats[it].format == VK_FORMAT_B8G8R8A8_SRGB && formats[it].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                {
                    return formats[it];
                }
            }
            return formats[0];
        };
        auto choosePresentationMode = [](ArrayView<VkPresentModeKHR> modes) -> VkPresentModeKHR
        {
            for (uSize it = 0; it < modes.count; it++)
            {
                // VK_PRESENT_MODE_MAILBOX_KHR will allow us to implemet triple buffering
                if (modes[it] == VK_PRESENT_MODE_MAILBOX_KHR) //VK_PRESENT_MODE_IMMEDIATE_KHR
                {
                    return modes[it];
                }
            }
            return VK_PRESENT_MODE_FIFO_KHR; // Guarateed to be available
        };
        auto chooseSwapExtent = [](RendererBackend* backend, VkSurfaceCapabilitiesKHR* capabilities) -> VkExtent2D
        {
            if (capabilities->currentExtent.width != UINT32_MAX)
            {
                return capabilities->currentExtent;
            }
            else
            {
                auto clamp = [](u32 min, u32 max, u32 value) -> u32 { return value < min ? min : (value > max ? max : value); };
                return
                {
                    clamp(capabilities->minImageExtent.width , capabilities->maxImageExtent.width , platform_window_get_current_width (backend->window)),
                    clamp(capabilities->minImageExtent.height, capabilities->maxImageExtent.height, platform_window_get_current_height(backend->window))
                };
            }
        };
        {
            SwapChainSupportDetails supportDetails = get_swap_chain_support_details(backend->surface, backend->gpu.physicalHandle, backend->memoryManager.cpu_allocationBindings);
            defer(av_destruct(supportDetails.formats));
            defer(av_destruct(supportDetails.presentModes));
            VkSurfaceFormatKHR  surfaceFormat   = chooseSurfaceFormat   (supportDetails.formats);
            VkPresentModeKHR    presentMode     = choosePresentationMode(supportDetails.presentModes);
            VkExtent2D          extent          = chooseSwapExtent      (backend, &supportDetails.capabilities);
            // @NOTE :  supportDetails.capabilities.maxImageCount == 0 means unlimited amount of images in swapchain
            u32 imageCount = supportDetails.capabilities.minImageCount + 1;
            if (supportDetails.capabilities.maxImageCount != 0 && imageCount > supportDetails.capabilities.maxImageCount)
            {
                imageCount = supportDetails.capabilities.maxImageCount;
            }
            // @NOTE :  imageSharingMode, queueFamilyIndexCount and pQueueFamilyIndices may vary depending on queue families indices.
            //          If the same queue family supports both graphics operations and presentation operations, we use exclusive sharing mode
            //          (which is more performance-friendly), but if there is two different queue families for these operations, we use
            //          concurrent sharing mode and pass indices array info to queueFamilyIndexCount and pQueueFamilyIndices
            VkSharingMode   imageSharingMode        = VK_SHARING_MODE_EXCLUSIVE;
            u32             queueFamilyIndexCount   = 0;
            const u32*      pQueueFamilyIndices     = nullptr;
            u32 queueFamiliesIndices[CommandQueues::QUEUES_NUM];
            for (uSize it = 0; it < CommandQueues::QUEUES_NUM; it++)
            {
                queueFamiliesIndices[it] = backend->queues.queues[it].deviceFamilyIndex;
            }
            bool isConcurrentSharingNeeded = false;
            for (uSize it = 1; it < CommandQueues::QUEUES_NUM; it++)
            {
                if (&backend->queues.queues[it] == &backend->queues.transfer)
                {
                    // ignore transfer queue
                    continue;
                }
                if (queueFamiliesIndices[it] != queueFamiliesIndices[0])
                {
                    isConcurrentSharingNeeded = true;
                    break;
                }
            }
            if (isConcurrentSharingNeeded)
            {
                // @NOTE :  if VK_SHARING_MODE_EXCLUSIVE was used here, we would have to explicitly transfer ownership of swap chain images
                //          from one queue to another
                imageSharingMode        = VK_SHARING_MODE_CONCURRENT;
                queueFamilyIndexCount   = CommandQueues::QUEUES_NUM;
                pQueueFamilyIndices     = queueFamiliesIndices;
            }
            VkSwapchainCreateInfoKHR createInfo
            {
                .sType                  = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
                .pNext                  = nullptr,
                .flags                  = 0,
                .surface                = backend->surface,
                .minImageCount          = imageCount,
                .imageFormat            = surfaceFormat.format,
                .imageColorSpace        = surfaceFormat.colorSpace,
                .imageExtent            = extent,
                .imageArrayLayers       = 1, // "This is always 1 unless you are developing a stereoscopic 3D application"
                .imageUsage             = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                .imageSharingMode       = imageSharingMode,
                .queueFamilyIndexCount  = queueFamilyIndexCount,
                .pQueueFamilyIndices    = pQueueFamilyIndices,
                .preTransform           = supportDetails.capabilities.currentTransform, // Allows to apply transform to images (rotate etc)
                .compositeAlpha         = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                .presentMode            = presentMode,
                .clipped                = VK_TRUE,
                .oldSwapchain           = VK_NULL_HANDLE,
            };
            u32 swapChainImageCount;
            al_vk_check(vkCreateSwapchainKHR(backend->gpu.logicalHandle, &createInfo, &backend->memoryManager.cpu_allocationCallbacks, &backend->swapChain.handle));
            al_vk_check(vkGetSwapchainImagesKHR(backend->gpu.logicalHandle, backend->swapChain.handle, &swapChainImageCount, nullptr));
            av_construct(&backend->swapChain.images, &backend->memoryManager.cpu_allocationBindings, swapChainImageCount);
            al_vk_check(vkGetSwapchainImagesKHR(backend->gpu.logicalHandle, backend->swapChain.handle, &swapChainImageCount, backend->swapChain.images.memory));
            backend->swapChain.format = surfaceFormat.format;
            backend->swapChain.extent = extent;
        }
        {
            av_construct(&backend->swapChain.imageViews, &backend->memoryManager.cpu_allocationBindings, backend->swapChain.images.count);
            for (uSize it = 0; it < backend->swapChain.imageViews.count; it++)
            {
                VkImageViewCreateInfo createInfo
                {
                    .sType              = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                    .pNext              = nullptr,
                    .flags              = 0,
                    .image              = backend->swapChain.images[it],
                    .viewType           = VK_IMAGE_VIEW_TYPE_2D,
                    .format             = backend->swapChain.format,
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
                al_vk_check(vkCreateImageView(backend->gpu.logicalHandle, &createInfo, &backend->memoryManager.cpu_allocationCallbacks, &backend->swapChain.imageViews[it]));
            }
        }
    }

    void create_depth_stencil(RendererBackend* backend)
    {
        VkImageCreateInfo imageCreateInfo = { };
        imageCreateInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCreateInfo.imageType     = VK_IMAGE_TYPE_2D;
        imageCreateInfo.format        = backend->depthStencil.format;
        imageCreateInfo.extent        = { backend->swapChain.extent.width, backend->swapChain.extent.height, 1 };
        imageCreateInfo.mipLevels     = 1;
        imageCreateInfo.arrayLayers   = 1;
        imageCreateInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
        imageCreateInfo.usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        al_vk_check(vkCreateImage(backend->gpu.logicalHandle, &imageCreateInfo, &backend->memoryManager.cpu_allocationCallbacks, &backend->depthStencil.image));

        VkMemoryRequirements memoryRequirements;
        vkGetImageMemoryRequirements(backend->gpu.logicalHandle, backend->depthStencil.image, &memoryRequirements);

        VkMemoryAllocateInfo memoryAllocateInfo = { };
        memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memoryAllocateInfo.allocationSize = memoryRequirements.size;
        bool result = vk::get_memory_type_index(&backend->gpu.memoryProperties, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memoryAllocateInfo.memoryTypeIndex);
        al_vk_assert(result);
        al_vk_check(vkAllocateMemory(backend->gpu.logicalHandle, &memoryAllocateInfo, &backend->memoryManager.cpu_allocationCallbacks, &backend->depthStencil.memory));
        al_vk_check(vkBindImageMemory(backend->gpu.logicalHandle, backend->depthStencil.image, backend->depthStencil.memory, 0));

        VkImageViewCreateInfo imageViewCreateInfo = { };
        imageViewCreateInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format                          = backend->depthStencil.format;
        imageViewCreateInfo.subresourceRange                = { };
        imageViewCreateInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel   = 0;
        imageViewCreateInfo.subresourceRange.levelCount     = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount     = 1;
        imageViewCreateInfo.image                           = backend->depthStencil.image;
        al_vk_check(vkCreateImageView(backend->gpu.logicalHandle, &imageViewCreateInfo, &backend->memoryManager.cpu_allocationCallbacks, &backend->depthStencil.view));
    }

    void create_framebuffers(RendererBackend* backend)
    {
        av_construct(&backend->swapChain.framebuffers, &backend->memoryManager.cpu_allocationBindings, backend->swapChain.images.count);
        for (uSize it = 0; it < backend->swapChain.images.count; it++)
        {
            VkImageView attachments[] =
            {
                backend->swapChain.imageViews[it],
                backend->depthStencil.view
            };
            VkFramebufferCreateInfo framebufferInfo
            {
                .sType              = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .pNext              = nullptr,
                .flags              = 0,
                .renderPass         = backend->simpleRenderPass,
                .attachmentCount    = array_size(attachments),
                .pAttachments       = attachments,
                .width              = backend->swapChain.extent.width,
                .height             = backend->swapChain.extent.height,
                .layers             = 1,
            };
            al_vk_check(vkCreateFramebuffer(backend->gpu.logicalHandle, &framebufferInfo, &backend->memoryManager.cpu_allocationCallbacks, &backend->swapChain.framebuffers[it]));
        }
    }

    void create_render_passes(RendererBackend* backend)
    {
        // Descriptors for the attachments used by this renderpass
        VkAttachmentDescription attachments[2] = { };

        // Color attachment
        attachments[0].format           = backend->swapChain.format;            // Use the color format selected by the swapchain
        attachments[0].samples          = VK_SAMPLE_COUNT_1_BIT;                // We don't use multi sampling in this example
        attachments[0].loadOp           = VK_ATTACHMENT_LOAD_OP_CLEAR;          // Clear this attachment at the start of the render pass
        attachments[0].storeOp          = VK_ATTACHMENT_STORE_OP_STORE;         // Keep its contents after the render pass is finished (for displaying it)
        attachments[0].stencilLoadOp    = VK_ATTACHMENT_LOAD_OP_DONT_CARE;      // We don't use stencil, so don't care for load
        attachments[0].stencilStoreOp   = VK_ATTACHMENT_STORE_OP_DONT_CARE;     // Same for store
        attachments[0].initialLayout    = VK_IMAGE_LAYOUT_UNDEFINED;            // Layout at render pass start. Initial doesn't matter, so we use undefined
        attachments[0].finalLayout      = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;      // Layout to which the attachment is transitioned when the render pass is finished
                                                                                // As we want to present the color buffer to the swapchain, we transition to PRESENT_KHR
        // Depth attachment
        attachments[1].format           = backend->depthStencil.format;
        attachments[1].samples          = VK_SAMPLE_COUNT_1_BIT;
        attachments[1].loadOp           = VK_ATTACHMENT_LOAD_OP_CLEAR;                      // Clear depth at start of first subpass
        attachments[1].storeOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;                 // We don't need depth after render pass has finished (DONT_CARE may result in better performance)
        attachments[1].stencilLoadOp    = VK_ATTACHMENT_LOAD_OP_DONT_CARE;                  // No stencil
        attachments[1].stencilStoreOp   = VK_ATTACHMENT_STORE_OP_DONT_CARE;                 // No Stencil
        attachments[1].initialLayout    = VK_IMAGE_LAYOUT_UNDEFINED;                        // Layout at render pass start. Initial doesn't matter, so we use undefined
        attachments[1].finalLayout      = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; // Transition to depth/stencil attachment

        // Setup attachment references
        VkAttachmentReference colorReference = { };
        colorReference.attachment = 0;                                    // Attachment 0 is color
        colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // Attachment layout used as color during the subpass

        VkAttachmentReference depthReference = { };
        depthReference.attachment = 1;                                            // Attachment 1 is color
        depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; // Attachment used as depth/stencil used during the subpass

        // Setup a single subpass reference
        VkSubpassDescription subpassDescription     = { };
        subpassDescription.pipelineBindPoint        = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescription.colorAttachmentCount     = 1;                                // Subpass uses one color attachment
        subpassDescription.pColorAttachments        = &colorReference;                  // Reference to the color attachment in slot 0
        subpassDescription.pDepthStencilAttachment  = &depthReference;                  // Reference to the depth attachment in slot 1
        subpassDescription.inputAttachmentCount     = 0;                                // Input attachments can be used to sample from contents of a previous subpass
        subpassDescription.pInputAttachments        = nullptr;                          // (Input attachments not used by this example)
        subpassDescription.preserveAttachmentCount  = 0;                                // Preserved attachments can be used to loop (and preserve) attachments through subpasses
        subpassDescription.pPreserveAttachments     = nullptr;                          // (Preserve attachments not used by this example)
        subpassDescription.pResolveAttachments      = nullptr;                          // Resolve attachments are resolved at the end of a sub pass and can be used for e.g. multi sampling

        // Setup subpass dependencies
        // These will add the implicit attachment layout transitions specified by the attachment descriptions
        // The actual usage layout is preserved through the layout specified in the attachment reference
        // Each subpass dependency will introduce a memory and execution dependency between the source and dest subpass described by
        // srcStageMask, dstStageMask, srcAccessMask, dstAccessMask (and dependencyFlags is set)
        // Note: VK_SUBPASS_EXTERNAL is a special constant that refers to all commands executed outside of the actual renderpass)
        VkSubpassDependency dependencies[2] = { };

        // First dependency at the start of the renderpass
        // Does the transition from final to initial layout
        dependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;                              // Producer of the dependency
        dependencies[0].dstSubpass      = 0;                                                // Consumer is our single subpass that will wait for the execution dependency
        dependencies[0].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;    // Match our pWaitDstStageMask when we vkQueueSubmit
        dependencies[0].dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;    // is a loadOp stage for color attachments
        dependencies[0].srcAccessMask   = 0;                                                // semaphore wait already does memory dependency for us
        dependencies[0].dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;             // is a loadOp CLEAR access mask for color attachments
        dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        // Second dependency at the end the renderpass
        // Does the transition from the initial to the final layout
        // Technically this is the same as the implicit subpass dependency, but we are gonna state it explicitly here
        dependencies[1].srcSubpass      = 0;                                                // Producer of the dependency is our single subpass
        dependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;                              // Consumer are all commands outside of the renderpass
        dependencies[1].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;    // is a storeOp stage for color attachments
        dependencies[1].dstStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;             // Do not block any subsequent work
        dependencies[1].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;             // is a storeOp `STORE` access mask for color attachments
        dependencies[1].dstAccessMask   = 0;
        dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        // Create the actual renderpass
        VkRenderPassCreateInfo renderPassInfo = { };
        renderPassInfo.sType            = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount  = array_size(attachments);                  // Number of attachments used by this render pass
        renderPassInfo.pAttachments     = attachments;                              // Descriptions of the attachments used by the render pass
        renderPassInfo.subpassCount     = 1;                                        // We only use one subpass in this example
        renderPassInfo.pSubpasses       = &subpassDescription;                      // Description of that subpass
        renderPassInfo.dependencyCount  = array_size(dependencies);                 // Number of subpass dependencies
        renderPassInfo.pDependencies    = dependencies;                             // Subpass dependencies used by the render pass

        al_vk_check(vkCreateRenderPass(backend->gpu.logicalHandle, &renderPassInfo, &backend->memoryManager.cpu_allocationCallbacks, &backend->simpleRenderPass));
    }

    void create_render_pipelines(RendererBackend* backend)
    {
        PlatformFile vertShader = platform_file_load(backend->memoryManager.cpu_allocationBindings, RendererBackend::VERTEX_SHADER_PATH, PlatformFileLoadMode::READ);
        PlatformFile fragShader = platform_file_load(backend->memoryManager.cpu_allocationBindings, RendererBackend::FRAGMENT_SHADER_PATH, PlatformFileLoadMode::READ);
        defer(platform_file_unload(backend->memoryManager.cpu_allocationBindings, vertShader));
        defer(platform_file_unload(backend->memoryManager.cpu_allocationBindings, fragShader));
        VkShaderModule vertShaderModule = vk::create_shader_module(backend->gpu.logicalHandle, { static_cast<u32*>(vertShader.memory), vertShader.sizeBytes }, &backend->memoryManager.cpu_allocationCallbacks);
        VkShaderModule fragShaderModule = vk::create_shader_module(backend->gpu.logicalHandle, { static_cast<u32*>(fragShader.memory), fragShader.sizeBytes }, &backend->memoryManager.cpu_allocationCallbacks);
        defer(vk::destroy_shader_module(backend->gpu.logicalHandle, vertShaderModule, &backend->memoryManager.cpu_allocationCallbacks));
        defer(vk::destroy_shader_module(backend->gpu.logicalHandle, fragShaderModule, &backend->memoryManager.cpu_allocationCallbacks));
        VkPipelineShaderStageCreateInfo shaderStages[] =
        {
            {
                .sType                  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext                  = nullptr,
                .flags                  = 0,
                .stage                  = VK_SHADER_STAGE_VERTEX_BIT,
                .module                 = vertShaderModule,
                .pName                  = "main",   // program entrypoint
                .pSpecializationInfo    = nullptr,  // values for shader constants
            },
            {
                .sType                  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext                  = nullptr,
                .flags                  = 0,
                .stage                  = VK_SHADER_STAGE_FRAGMENT_BIT,
                .module                 = fragShaderModule,
                .pName                  = "main",   // program entrypoint
                .pSpecializationInfo    = nullptr,  // values for shader constants
            },
        };
        VkVertexInputBindingDescription bindingDescriptions[] =
        {
            {
                .binding    = 0,
                .stride     = sizeof(RenderVertex),
                .inputRate  = VK_VERTEX_INPUT_RATE_VERTEX,
            }
        };
        VkVertexInputAttributeDescription attributeDescriptions[]
        {
            {
                .location   = 0,
                .binding    = 0,
                .format     = VK_FORMAT_R32G32B32_SFLOAT,
                .offset     = offsetof(RenderVertex, position),
            },
            {
                .location   = 1,
                .binding    = 0,
                .format     = VK_FORMAT_R32G32B32_SFLOAT,
                .offset     = offsetof(RenderVertex, normal),
            },
            {
                .location   = 2,
                .binding    = 0,
                .format     = VK_FORMAT_R32G32_SFLOAT,
                .offset     = offsetof(RenderVertex, uv),
            },
        };
        VkPipelineVertexInputStateCreateInfo vertexInputInfo
        {
            .sType                              = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .pNext                              = nullptr,
            .flags                              = 0,
            .vertexBindingDescriptionCount      = array_size(bindingDescriptions),
            .pVertexBindingDescriptions         = bindingDescriptions,
            .vertexAttributeDescriptionCount    = array_size(attributeDescriptions),
            .pVertexAttributeDescriptions       = attributeDescriptions,
        };
        VkPipelineInputAssemblyStateCreateInfo inputAssembly
        {
            .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .pNext                  = nullptr,
            .flags                  = 0,
            .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = VK_FALSE,
        };
        VkViewport viewport
        {
            .x          = 0.0f,
            .y          = 0.0f,
            .width      = static_cast<float>(backend->swapChain.extent.width),
            .height     = static_cast<float>(backend->swapChain.extent.height),
            .minDepth   = 0.0f,
            .maxDepth   = 1.0f,
        };
        VkRect2D scissor
        {
            .offset = { 0, 0 },
            .extent = backend->swapChain.extent,
        };
        VkPipelineViewportStateCreateInfo viewportState
        {
            .sType          = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .pNext          = nullptr,
            .flags          = 0,
            .viewportCount  = 1,
            .pViewports     = &viewport,
            .scissorCount   = 1,
            .pScissors      = &scissor,
        };
        VkPipelineRasterizationStateCreateInfo rasterizer
        {
            .sType                      = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .pNext                      = nullptr,
            .flags                      = 0,
            .depthClampEnable           = VK_FALSE,
            .rasterizerDiscardEnable    = VK_FALSE,
            .polygonMode                = VK_POLYGON_MODE_FILL,
            .cullMode                   = VK_CULL_MODE_BACK_BIT,
            .frontFace                  = VK_FRONT_FACE_CLOCKWISE,
            .depthBiasEnable            = VK_FALSE,
            .depthBiasConstantFactor    = 0.0f,
            .depthBiasClamp             = 0.0f,
            .depthBiasSlopeFactor       = 0.0f,
            .lineWidth                  = 1.0f,
        };
        VkPipelineMultisampleStateCreateInfo multisampling
        {
            .sType                  = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .pNext                  = nullptr,
            .flags                  = 0,
            .rasterizationSamples   = VK_SAMPLE_COUNT_1_BIT,
            .sampleShadingEnable    = VK_FALSE,
            .minSampleShading       = 1.0f,
            .pSampleMask            = nullptr,
            .alphaToCoverageEnable  = VK_FALSE,
            .alphaToOneEnable       = VK_FALSE,
        };
        VkStencilOpState stencilOpState{ };
        stencilOpState.failOp       = VK_STENCIL_OP_KEEP;
        stencilOpState.passOp       = VK_STENCIL_OP_KEEP;
        stencilOpState.compareOp    = VK_COMPARE_OP_ALWAYS;
        VkPipelineDepthStencilStateCreateInfo depthStencil
        {
            .sType                  = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .pNext                  = nullptr,
            .flags                  = 0,
            .depthTestEnable        = VK_TRUE,
            .depthWriteEnable       = VK_TRUE,
            .depthCompareOp         = VK_COMPARE_OP_LESS_OR_EQUAL,
            .depthBoundsTestEnable  = VK_FALSE,
            .stencilTestEnable      = VK_FALSE,
            .front                  = stencilOpState,
            .back                   = stencilOpState,
            .minDepthBounds         = 0.0f,
            .maxDepthBounds         = 0.0f,
        };
        VkPipelineColorBlendAttachmentState colorBlendAttachments[] =
        {
            {
                .blendEnable            = VK_FALSE,
                .srcColorBlendFactor    = VK_BLEND_FACTOR_ONE,  // optional if blendEnable == VK_FALSE
                .dstColorBlendFactor    = VK_BLEND_FACTOR_ZERO, // optional if blendEnable == VK_FALSE
                .colorBlendOp           = VK_BLEND_OP_ADD,      // optional if blendEnable == VK_FALSE
                .srcAlphaBlendFactor    = VK_BLEND_FACTOR_ONE,  // optional if blendEnable == VK_FALSE
                .dstAlphaBlendFactor    = VK_BLEND_FACTOR_ZERO, // optional if blendEnable == VK_FALSE
                .alphaBlendOp           = VK_BLEND_OP_ADD,      // optional if blendEnable == VK_FALSE
                .colorWriteMask         = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
            }
        };
        VkPipelineColorBlendStateCreateInfo colorBlending
        {
            .sType              = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .pNext              = nullptr,
            .flags              = 0,
            .logicOpEnable      = VK_FALSE,
            .logicOp            = VK_LOGIC_OP_COPY, // optional if logicOpEnable == VK_FALSE
            .attachmentCount    = array_size(colorBlendAttachments),
            .pAttachments       = colorBlendAttachments,
            .blendConstants     = { 0.0f, 0.0f, 0.0f, 0.0f }, // optional, because color blending is disabled in colorBlendAttachments
        };
        VkDynamicState dynamicStates[] =
        {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamicState
        {
            .sType              = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .pNext              = nullptr,
            .flags              = 0,
            .dynamicStateCount  = sizeof(dynamicStates) / sizeof(dynamicStates[0]),
            .pDynamicStates     = dynamicStates,
        };
        // @TODO : VkDescriptorSetLayout
        // @TODO : VkDescriptorSetLayout
        // @TODO : VkDescriptorSetLayout
        // @TODO : VkDescriptorSetLayout
        // @TODO : VkDescriptorSetLayout
        // @TODO : VkDescriptorSetLayout
        // @TODO : VkDescriptorSetLayout
        // @TODO : VkDescriptorSetLayout
        // @TODO : VkDescriptorSetLayout
        // @TODO : VkDescriptorSetLayout
        // @TODO : VkDescriptorSetLayout
        // @TODO : VkDescriptorSetLayout
        // @TODO : VkDescriptorSetLayout
        // @TODO : VkDescriptorSetLayout
        // @TODO : VkDescriptorSetLayout
        // @TODO : VkDescriptorSetLayout
        // @TODO : VkDescriptorSetLayout
        // @TODO : VkDescriptorSetLayout
        // @TODO : VkDescriptorSetLayout
        // @TODO : VkDescriptorSetLayout
        // @TODO : VkDescriptorSetLayout
        // @TODO : VkDescriptorSetLayout
        // @TODO : VkDescriptorSetLayout
        // @TODO : VkDescriptorSetLayout
        // @TODO : VkDescriptorSetLayout
        // @TODO : VkDescriptorSetLayout
        // @TODO : VkDescriptorSetLayout
        VkPipelineLayoutCreateInfo pipelineLayoutInfo
        {
            .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext                  = nullptr,
            .flags                  = 0,
            .setLayoutCount         = 0,
            .pSetLayouts            = nullptr, //&backend->vkDescriptorSetLayout,
            .pushConstantRangeCount = 0,
            .pPushConstantRanges    = nullptr,
        };
        al_vk_check(vkCreatePipelineLayout(backend->gpu.logicalHandle, &pipelineLayoutInfo, &backend->memoryManager.cpu_allocationCallbacks, &backend->pipelineLayout));
        VkGraphicsPipelineCreateInfo pipelineInfo
        {
            .sType                  = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext                  = nullptr,
            .flags                  = 0,
            .stageCount             = 2,
            .pStages                = shaderStages,
            .pVertexInputState      = &vertexInputInfo,
            .pInputAssemblyState    = &inputAssembly,
            .pTessellationState     = nullptr,
            .pViewportState         = &viewportState,
            .pRasterizationState    = &rasterizer,
            .pMultisampleState      = &multisampling,
            .pDepthStencilState     = &depthStencil,
            .pColorBlendState       = &colorBlending,
            .pDynamicState          = &dynamicState,
            .layout                 = backend->pipelineLayout,
            .renderPass             = backend->simpleRenderPass,
            .subpass                = 0,
            .basePipelineHandle     = VK_NULL_HANDLE,
            .basePipelineIndex      = -1,
        };
        al_vk_check(vkCreateGraphicsPipelines(backend->gpu.logicalHandle, VK_NULL_HANDLE, 1, &pipelineInfo, &backend->memoryManager.cpu_allocationCallbacks, &backend->pipeline));
    }

    void create_sync_primitives(RendererBackend* backend)
    {
        // Semaphores
        av_construct(&backend->syncPrimitives.imageAvailableSemaphores, &backend->memoryManager.cpu_allocationBindings, vk::MAX_IMAGES_IN_FLIGHT);
        av_construct(&backend->syncPrimitives.renderFinishedSemaphores, &backend->memoryManager.cpu_allocationBindings, vk::MAX_IMAGES_IN_FLIGHT);
        for (uSize it = 0; it < vk::MAX_IMAGES_IN_FLIGHT; it++)
        {
            VkSemaphoreCreateInfo semaphoreInfo
            {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                .pNext = nullptr,
                .flags = { },
            };
            al_vk_check(vkCreateSemaphore(backend->gpu.logicalHandle, &semaphoreInfo, &backend->memoryManager.cpu_allocationCallbacks, &backend->syncPrimitives.imageAvailableSemaphores[it]));
            al_vk_check(vkCreateSemaphore(backend->gpu.logicalHandle, &semaphoreInfo, &backend->memoryManager.cpu_allocationCallbacks, &backend->syncPrimitives.renderFinishedSemaphores[it]));
        }
        // Fences
        av_construct(&backend->syncPrimitives.inFlightFences, &backend->memoryManager.cpu_allocationBindings, vk::MAX_IMAGES_IN_FLIGHT);
        for (uSize it = 0; it < vk::MAX_IMAGES_IN_FLIGHT; it++)
        {
            VkFenceCreateInfo fenceInfo
            {
                .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                .pNext = nullptr,
                .flags = VK_FENCE_CREATE_SIGNALED_BIT,
            };
            al_vk_check(vkCreateFence(backend->gpu.logicalHandle, &fenceInfo, &backend->memoryManager.cpu_allocationCallbacks, &backend->syncPrimitives.inFlightFences[it]));
        }
        av_construct(&backend->syncPrimitives.imageInFlightFencesRef, &backend->memoryManager.cpu_allocationBindings, backend->swapChain.images.count);
        for (uSize it = 0; it < backend->syncPrimitives.imageInFlightFencesRef.count; it++)
        {
            backend->syncPrimitives.imageInFlightFencesRef[it] = VK_NULL_HANDLE;
        }
    }

    // =================================================================================================================
    // DESTRUCTION
    // =================================================================================================================

    void destroy_instance(RendererBackend* backend)
    {
        auto DestroyDebugUtilsMessengerEXT = [](VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks* pAllocator)
        {
            PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
            if (func)
            {
                func(instance, messenger, pAllocator);
            }
        };
        DestroyDebugUtilsMessengerEXT(backend->instance, backend->debugMessenger, &backend->memoryManager.cpu_allocationCallbacks);
        vkDestroyInstance(backend->instance, &backend->memoryManager.cpu_allocationCallbacks);
    }

    void destroy_surface(RendererBackend* backend)
    {
        vkDestroySurfaceKHR(backend->instance, backend->surface, &backend->memoryManager.cpu_allocationCallbacks);
    }

    void destroy_gpu(RendererBackend* backend)
    {
        vkDestroyDevice(backend->gpu.logicalHandle, &backend->memoryManager.cpu_allocationCallbacks);
    }

    void destroy_swap_chain(RendererBackend* backend)
    {
        for (uSize it = 0; it < backend->swapChain.imageViews.count; it++)
        {
            vkDestroyImageView(backend->gpu.logicalHandle, backend->swapChain.imageViews[it], &backend->memoryManager.cpu_allocationCallbacks);
        }
        vkDestroySwapchainKHR(backend->gpu.logicalHandle, backend->swapChain.handle, &backend->memoryManager.cpu_allocationCallbacks);
        av_destruct(backend->swapChain.imageViews);
        av_destruct(backend->swapChain.images);
    }

    void destroy_depth_stencil(RendererBackend* backend)
    {
        vkDestroyImageView(backend->gpu.logicalHandle, backend->depthStencil.view, &backend->memoryManager.cpu_allocationCallbacks);
        vkDestroyImage(backend->gpu.logicalHandle, backend->depthStencil.image, &backend->memoryManager.cpu_allocationCallbacks);
        vkFreeMemory(backend->gpu.logicalHandle, backend->depthStencil.memory, &backend->memoryManager.cpu_allocationCallbacks);
    }

    void destroy_framebuffers(RendererBackend* backend)
    {
        for (uSize it = 0; it < backend->swapChain.framebuffers.count; it++)
        {
            vkDestroyFramebuffer(backend->gpu.logicalHandle, backend->swapChain.framebuffers[it], &backend->memoryManager.cpu_allocationCallbacks);
        }
        av_destruct(backend->swapChain.framebuffers);
    }

    void destroy_render_passes(RendererBackend* backend)
    {
        vkDestroyRenderPass(backend->gpu.logicalHandle, backend->simpleRenderPass, &backend->memoryManager.cpu_allocationCallbacks);
    }

    void destroy_render_pipelines(RendererBackend* backend)
    {
        vkDestroyPipeline(backend->gpu.logicalHandle, backend->pipeline, &backend->memoryManager.cpu_allocationCallbacks);
        vkDestroyPipelineLayout(backend->gpu.logicalHandle, backend->pipelineLayout, &backend->memoryManager.cpu_allocationCallbacks);
    }

    void destroy_sync_primitives(RendererBackend* backend)
    {
        // Semaphores
        for (uSize it = 0; it < vk::MAX_IMAGES_IN_FLIGHT; it++)
        {
            vkDestroySemaphore(backend->gpu.logicalHandle, backend->syncPrimitives.imageAvailableSemaphores[it], &backend->memoryManager.cpu_allocationCallbacks);
            vkDestroySemaphore(backend->gpu.logicalHandle, backend->syncPrimitives.renderFinishedSemaphores[it], &backend->memoryManager.cpu_allocationCallbacks);
        }
        av_destruct(backend->syncPrimitives.imageAvailableSemaphores);
        av_destruct(backend->syncPrimitives.renderFinishedSemaphores);
        // Fences
        for (uSize it = 0; it < vk::MAX_IMAGES_IN_FLIGHT; it++)
        {
            vkDestroyFence(backend->gpu.logicalHandle, backend->syncPrimitives.inFlightFences[it], &backend->memoryManager.cpu_allocationCallbacks);
        }
        av_destruct(backend->syncPrimitives.inFlightFences);
        av_destruct(backend->syncPrimitives.imageInFlightFencesRef);
    }






















    SwapChainSupportDetails get_swap_chain_support_details(VkSurfaceKHR surface, VkPhysicalDevice device, AllocatorBindings bindings)
    {
        SwapChainSupportDetails result{ };
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &result.capabilities);
        u32 count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, nullptr);
        if (count != 0)
        {
            av_construct(&result.formats, &bindings, count);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &count, result.formats.memory);
        }
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, nullptr);
        if (count != 0)
        {
            av_construct(&result.presentModes, &bindings, count);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &count, result.presentModes.memory);
        }
        return result;
    }

    void construct(VulkanMemoryManager* memoryManager, AllocatorBindings bindings)
    {
        std::memset(memoryManager, 0, sizeof(VulkanMemoryManager));
        memoryManager->cpu_allocationBindings = bindings;
        memoryManager->cpu_allocationCallbacks =
        {
            .pUserData = memoryManager,
            .pfnAllocation = 
                [](void* pUserData, uSize size, uSize alignment, VkSystemAllocationScope allocationScope)
                {
                    VulkanMemoryManager* manager = static_cast<VulkanMemoryManager*>(pUserData);
                    void* result = allocate(&manager->cpu_allocationBindings, size);
                    al_vk_assert(manager->cpu_currentNumberOfAllocations < VulkanMemoryManager::MAX_CPU_ALLOCATIONS);
                    manager->cpu_allocations[manager->cpu_currentNumberOfAllocations++] =
                    {
                        .ptr = result,
                        .size = size,
                    };
                    return result;
                },
            .pfnReallocation =
                [](void* pUserData, void* pOriginal, uSize size, uSize alignment, VkSystemAllocationScope allocationScope)
                {
                    VulkanMemoryManager* manager = static_cast<VulkanMemoryManager*>(pUserData);
                    void* result = nullptr;
                    for (uSize it = 0; it < manager->cpu_currentNumberOfAllocations; it++)
                    {
                        if (manager->cpu_allocations[it].ptr == pOriginal)
                        {
                            deallocate(&manager->cpu_allocationBindings, manager->cpu_allocations[it].ptr, manager->cpu_allocations[it].size);
                            result = allocate(&manager->cpu_allocationBindings, size);
                            manager->cpu_allocations[it] =
                            {
                                .ptr = result,
                                .size = size,
                            };
                            break;
                        }
                    }
                    return result;
                },
            .pfnFree =
                [](void* pUserData, void* pMemory)
                {
                    VulkanMemoryManager* manager = static_cast<VulkanMemoryManager*>(pUserData);
                    for (uSize it = 0; it < manager->cpu_currentNumberOfAllocations; it++)
                    {
                        if (manager->cpu_allocations[it].ptr == pMemory)
                        {
                            deallocate(&manager->cpu_allocationBindings, manager->cpu_allocations[it].ptr, manager->cpu_allocations[it].size);
                            manager->cpu_allocations[it] = manager->cpu_allocations[manager->cpu_currentNumberOfAllocations - 1];
                            manager->cpu_currentNumberOfAllocations -= 1;
                            break;
                        }
                    }
                },
            .pfnInternalAllocation  = nullptr,
            .pfnInternalFree        = nullptr
        };
        av_construct(&memoryManager->gpu_chunks, &bindings, VulkanMemoryManager::GPU_MAX_CHUNKS);
        av_construct(&memoryManager->gpu_ledgers, &bindings, VulkanMemoryManager::GPU_LEDGER_SIZE_BYTES * VulkanMemoryManager::GPU_MAX_CHUNKS);
        for (uSize it = 0; it < VulkanMemoryManager::GPU_MAX_CHUNKS; it++)
        {
            VulkanMemoryManager::GpuMemoryChunk* chunk = &memoryManager->gpu_chunks[it];
            chunk->ledger = &memoryManager->gpu_ledgers[it * VulkanMemoryManager::GPU_LEDGER_SIZE_BYTES];
        }
    }

    void destruct(VulkanMemoryManager* memoryManager, VkDevice device)
    {
        for (uSize it = 0; it < VulkanMemoryManager::GPU_MAX_CHUNKS; it++)
        {
            VulkanMemoryManager::GpuMemoryChunk* chunk = &memoryManager->gpu_chunks[it];
            if (chunk->memory)
            {
                vkFreeMemory(device, chunk->memory, &memoryManager->cpu_allocationCallbacks);
            }
        }
        av_destruct(memoryManager->gpu_chunks);
        av_destruct(memoryManager->gpu_ledgers);
    }

    GpuMemory gpu_allocate(VulkanMemoryManager* memoryManager, VkDevice device, VulkanMemoryManager::GpuAllocationRequest request)
    {
        auto findFreeSpace = [](VulkanMemoryManager::GpuMemoryChunk* chunk, uSize numBlocks) -> uSize
        {
            uSize freeCount = 0;
            uSize startBlock = 0;
            uSize currentBlock = 0;
            for (uSize ledgerIt = 0; ledgerIt < VulkanMemoryManager::GPU_LEDGER_SIZE_BYTES; ledgerIt++)
            {
                u8 byte = chunk->ledger[ledgerIt];
                if (byte == 255)
                {
                    freeCount = 0;
                    currentBlock += 8;
                    continue;
                }
                for (uSize byteIt = 0; byteIt < 8; byteIt++)
                {
                    if (byte & (1 << byteIt))
                    {
                        freeCount = 0;
                    }
                    else
                    {
                        if (freeCount == 0)
                        {
                            startBlock = currentBlock;
                        }
                        freeCount += 1;
                    }
                    currentBlock += 1;
                    if (freeCount == numBlocks)
                    {
                        goto block_found;
                    }
                }
            }
            startBlock = VulkanMemoryManager::GPU_LEDGER_SIZE_BYTES;
            block_found:
            return startBlock;
        };
        auto setInUse = [](VulkanMemoryManager::GpuMemoryChunk* chunk, uSize numBlocks, uSize startBlock)
        {
            for (uSize it = 0; it < numBlocks; it++)
            {
                uSize currentByte = (startBlock + it) / 8;
                uSize currentBit = (startBlock + it) % 8;
                u8* byte = &chunk->ledger[currentByte];
                *byte |= (1 << currentBit);
            }
            chunk->usedMemoryBytes += numBlocks * VulkanMemoryManager::GPU_MEMORY_BLOCK_SIZE_BYTES;
            al_vk_assert(chunk->usedMemoryBytes <= VulkanMemoryManager::GPU_CHUNK_SIZE_BYTES);
        };
        al_vk_assert(request.sizeBytes <= VulkanMemoryManager::GPU_CHUNK_SIZE_BYTES);
        GpuMemory result{ };
        uSize requiredNumberOfBlocks = 1 + ((request.sizeBytes - 1) / VulkanMemoryManager::GPU_MEMORY_BLOCK_SIZE_BYTES);
        // 1. Try to find memory in available chunks
        for (uSize it = 0; it < VulkanMemoryManager::GPU_MAX_CHUNKS; it++)
        {
            VulkanMemoryManager::GpuMemoryChunk* chunk = &memoryManager->gpu_chunks[it];
            if (!chunk->memory)
            {
                // Chunk is not allocated
                continue;
            }
            if (chunk->memoryTypeIndex != request.memoryTypeIndex)
            {
                // Chunk has wrong memory type
                continue;
            }
            if ((VulkanMemoryManager::GPU_CHUNK_SIZE_BYTES - chunk->usedMemoryBytes) < request.sizeBytes)
            {
                // Chunk does not have enough memory
                continue;
            }
            uSize inChunkOffset = findFreeSpace(chunk, requiredNumberOfBlocks);
            assert(inChunkOffset != VulkanMemoryManager::GPU_LEDGER_SIZE_BYTES);
            setInUse(chunk, requiredNumberOfBlocks, inChunkOffset);
            result.memory = chunk->memory;
            result.offsetBytes = inChunkOffset * VulkanMemoryManager::GPU_MEMORY_BLOCK_SIZE_BYTES;
            result.sizeBytes = requiredNumberOfBlocks * VulkanMemoryManager::GPU_MEMORY_BLOCK_SIZE_BYTES;
            break;
        }
        if (result.memory)
        {
            // Found free space in already allocated chunk
            return result;
        }
        // 2. Try to allocate new chunk
        for (uSize it = 0; it < VulkanMemoryManager::GPU_MAX_CHUNKS; it++)
        {
            VulkanMemoryManager::GpuMemoryChunk* chunk = &memoryManager->gpu_chunks[it];
            if (chunk->memory)
            {
                continue;
            }
            // Allocating new chunk
            VkMemoryAllocateInfo memoryAllocateInfo = { };
            memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            memoryAllocateInfo.allocationSize = VulkanMemoryManager::GPU_CHUNK_SIZE_BYTES;
            memoryAllocateInfo.memoryTypeIndex = request.memoryTypeIndex;
            al_vk_check(vkAllocateMemory(device, &memoryAllocateInfo, &memoryManager->cpu_allocationCallbacks, &chunk->memory));
            chunk->memoryTypeIndex = request.memoryTypeIndex;
            std::memset(chunk->ledger, 0, VulkanMemoryManager::GPU_LEDGER_SIZE_BYTES);
            // Allocating memory in new chunk
            uSize inChunkOffset = findFreeSpace(chunk, requiredNumberOfBlocks);
            al_vk_assert(inChunkOffset != VulkanMemoryManager::GPU_LEDGER_SIZE_BYTES);
            setInUse(chunk, requiredNumberOfBlocks, inChunkOffset);
            result.memory = chunk->memory;
            result.offsetBytes = inChunkOffset * VulkanMemoryManager::GPU_MEMORY_BLOCK_SIZE_BYTES;
            result.sizeBytes = requiredNumberOfBlocks * VulkanMemoryManager::GPU_MEMORY_BLOCK_SIZE_BYTES;
            break;
        }
        al_vk_assert(result.memory); // Out of memory
        return result;
    }

    void gpu_deallocate(VulkanMemoryManager* memoryManager, VkDevice device, GpuMemory allocation)
    {
        auto setFree = [](VulkanMemoryManager::GpuMemoryChunk* chunk, uSize numBlocks, uSize startBlock)
        {
            for (uSize it = 0; it < numBlocks; it++)
            {
                uSize currentByte = (startBlock + it) / 8;
                uSize currentBit = (startBlock + it) % 8;
                u8* byte = &chunk->ledger[currentByte];
                *byte &= ~(1 << currentBit);
            }
            chunk->usedMemoryBytes -= numBlocks * VulkanMemoryManager::GPU_MEMORY_BLOCK_SIZE_BYTES;
            al_vk_assert(chunk->usedMemoryBytes <= VulkanMemoryManager::GPU_CHUNK_SIZE_BYTES);
        };
        for (uSize it = 0; it < VulkanMemoryManager::GPU_MAX_CHUNKS; it++)
        {
            VulkanMemoryManager::GpuMemoryChunk* chunk = &memoryManager->gpu_chunks[it];
            if (chunk->memory != allocation.memory)
            {
                continue;
            }
            uSize startBlock = allocation.offsetBytes / VulkanMemoryManager::GPU_MEMORY_BLOCK_SIZE_BYTES;
            uSize numBlocks = allocation.sizeBytes / VulkanMemoryManager::GPU_MEMORY_BLOCK_SIZE_BYTES;
            setFree(chunk, numBlocks, startBlock);
            if (chunk->usedMemoryBytes == 0)
            {
                // Chunk is empty, so we free it
                vkFreeMemory(device, chunk->memory, &memoryManager->cpu_allocationCallbacks);
                chunk->memory = VK_NULL_HANDLE;
            }
            break;
        }
    }

    MemoryBuffer create_buffer(RendererBackend* backend, VkBufferCreateInfo* createInfo, VkMemoryPropertyFlags memoryProperty)
    {
        MemoryBuffer buffer{ };
        al_vk_check(vkCreateBuffer(backend->gpu.logicalHandle, createInfo, &backend->memoryManager.cpu_allocationCallbacks, &buffer.handle));
        VkMemoryRequirements memoryRequirements;
        vkGetBufferMemoryRequirements(backend->gpu.logicalHandle, buffer.handle, &memoryRequirements);
        u32 memoryTypeIndex;
        vk::get_memory_type_index(&backend->gpu.memoryProperties, memoryRequirements.memoryTypeBits, memoryProperty, &memoryTypeIndex);
        buffer.gpuMemory = gpu_allocate(&backend->memoryManager, backend->gpu.logicalHandle, { .sizeBytes = memoryRequirements.size, .memoryTypeIndex = memoryTypeIndex });
        al_vk_check(vkBindBufferMemory(backend->gpu.logicalHandle, buffer.handle, buffer.gpuMemory.memory, buffer.gpuMemory.offsetBytes));
        return buffer;
    }

    MemoryBuffer create_vertex_buffer(RendererBackend* backend, uSize sizeSytes)
    {
        VkBufferCreateInfo bufferInfo
        {
            .sType                  = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext                  = nullptr,
            .flags                  = 0,
            .size                   = sizeSytes,
            .usage                  = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            .sharingMode            = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount  = 0,        // ignored if sharingMode is not VK_SHARING_MODE_CONCURRENT
            .pQueueFamilyIndices    = nullptr,  // ignored if sharingMode is not VK_SHARING_MODE_CONCURRENT
        };
        u32 queueFamilyIndices[] =
        {
            backend->queues.transfer.deviceFamilyIndex,
            backend->queues.graphics.deviceFamilyIndex
        };
        if (backend->queues.transfer.deviceFamilyIndex != backend->queues.graphics.deviceFamilyIndex)
        {
            bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
            bufferInfo.queueFamilyIndexCount = 2;
            bufferInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        return create_buffer(backend, &bufferInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }

    MemoryBuffer create_index_buffer(RendererBackend* backend, uSize sizeSytes)
    {
        VkBufferCreateInfo bufferInfo
        {
            .sType                  = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext                  = nullptr,
            .flags                  = 0,
            .size                   = sizeSytes,
            .usage                  = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            .sharingMode            = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount  = 0,        // ignored if sharingMode is not VK_SHARING_MODE_CONCURRENT
            .pQueueFamilyIndices    = nullptr,  // ignored if sharingMode is not VK_SHARING_MODE_CONCURRENT
        };
        u32 queueFamilyIndices[] =
        {
            backend->queues.transfer.deviceFamilyIndex,
            backend->queues.graphics.deviceFamilyIndex
        };
        if (backend->queues.transfer.deviceFamilyIndex != backend->queues.graphics.deviceFamilyIndex)
        {
            bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
            bufferInfo.queueFamilyIndexCount = 2;
            bufferInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        return create_buffer(backend, &bufferInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }

    MemoryBuffer create_staging_buffer(RendererBackend* backend, uSize sizeSytes)
    {
        VkBufferCreateInfo bufferInfo
        {
            .sType                  = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext                  = nullptr,
            .flags                  = 0,
            .size                   = sizeSytes,
            .usage                  = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .sharingMode            = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount  = 0,        // ignored if sharingMode is not VK_SHARING_MODE_CONCURRENT
            .pQueueFamilyIndices    = nullptr,  // ignored if sharingMode is not VK_SHARING_MODE_CONCURRENT
        };
        return create_buffer(backend, &bufferInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }

    void destroy_buffer(RendererBackend* backend, MemoryBuffer* buffer)
    {
        gpu_deallocate(&backend->memoryManager, backend->gpu.logicalHandle, buffer->gpuMemory);
        vkDestroyBuffer(backend->gpu.logicalHandle, buffer->handle, &backend->memoryManager.cpu_allocationCallbacks);
        std::memset(&buffer, 0, sizeof(MemoryBuffer));
    }

    void copy_cpu_memory_to_buffer(RendererBackend* backend, void* data, MemoryBuffer* buffer, uSize dataSizeBytes)
    {
        void* mappedMemory;
        vkMapMemory(backend->gpu.logicalHandle, buffer->gpuMemory.memory, buffer->gpuMemory.offsetBytes, dataSizeBytes, 0, &mappedMemory);
        std::memcpy(mappedMemory, data, dataSizeBytes);
        vkUnmapMemory(backend->gpu.logicalHandle, buffer->gpuMemory.memory);
    }

    void copy_buffer_to_buffer(RendererBackend* backend, MemoryBuffer* src, MemoryBuffer* dst, uSize sizeBytes, uSize srcOffsetBytes, uSize dstOffsetBytes)
    {
        CommandBuffers* buffers = get_command_buffers(backend);
        VkCommandBufferBeginInfo beginInfo
        {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext              = nullptr,
            .flags              = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            .pInheritanceInfo   = nullptr,
        };
        vkBeginCommandBuffer(buffers->transferBuffer, &beginInfo);
        VkBufferCopy copyRegion
        {
            .srcOffset  = srcOffsetBytes,
            .dstOffset  = dstOffsetBytes,
            .size       = sizeBytes,
        };
        vkCmdCopyBuffer(buffers->transferBuffer, src->handle, dst->handle, 1, &copyRegion);
        vkEndCommandBuffer(buffers->transferBuffer);
        VkSubmitInfo submitInfo
        {
            .sType                  = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext                  = nullptr,
            .waitSemaphoreCount     = 0,
            .pWaitSemaphores        = nullptr,
            .pWaitDstStageMask      = 0,
            .commandBufferCount     = 1,
            .pCommandBuffers        = &buffers->transferBuffer,
            .signalSemaphoreCount   = 0,
            .pSignalSemaphores      = nullptr,
        };
        vkQueueSubmit(backend->queues.transfer.handle, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(backend->queues.transfer.handle);
        // vkFreeCommandBuffers(backend->gpu.logicalHandle, backend->commandPools.transfer.handle, 1, &commandBuffer);
    }

    bool is_command_queues_complete(CommandQueues* queues)
    {
        for (uSize it = 0; it < CommandQueues::QUEUES_NUM; it++)
        {
            if (!queues->queues[it].isFamilyPresent)
            {
                return false;
            }
        }
        return true;
    };

    void try_pick_graphics_queue(CommandQueues::Queue* queue, ArrayView<VkQueueFamilyProperties> familyProperties)
    {
        for (u32 it = 0; it < familyProperties.count; it++)
        {
            if (familyProperties[it].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                queue->deviceFamilyIndex = it;
                queue->isFamilyPresent = true;
                break;
            }
        }
    }

    void try_pick_present_queue(CommandQueues::Queue* queue, ArrayView<VkQueueFamilyProperties> familyProperties, VkPhysicalDevice device, VkSurfaceKHR surface)
    {
        for (u32 it = 0; it < familyProperties.count; it++)
        {
            VkBool32 isSupported;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, it, surface, &isSupported);
            if (isSupported)
            {
                queue->deviceFamilyIndex = it;
                queue->isFamilyPresent = true;
                break;
            }
        }
    }

    void try_pick_transfer_queue(CommandQueues::Queue* queue, ArrayView<VkQueueFamilyProperties> familyProperties, u32 graphicsFamilyIndex)
    {
        // First try to pick queue that is different from the graphics queue
        for (u32 it = 0; it < familyProperties.count; it++)
        {
            if (it == graphicsFamilyIndex)
            {
                continue;
            }
            if (familyProperties[it].queueFlags & VK_QUEUE_TRANSFER_BIT)
            {
                queue->deviceFamilyIndex = it;
                queue->isFamilyPresent = true;
                return;
            }
        }
        // If there is no separate queue family, use graphics family (if possible (propbably this is possible almost always))
        if (familyProperties[graphicsFamilyIndex].queueFlags & VK_QUEUE_TRANSFER_BIT)
        {
            queue->deviceFamilyIndex = graphicsFamilyIndex;
            queue->isFamilyPresent = true;
        }
    }


    template<typename T>
    void tls_construct(ThreadLocalStorage<T>* storage, AllocatorBindings bindings)
    {
        storage->bindings = bindings;
        storage->capacity = ThreadLocalStorage<T>::DEFAULT_CAPACITY;
        storage->size = 0;
        storage->memory = static_cast<T*>(allocate(&storage->bindings, storage->capacity * sizeof(T)));
        std::memset(storage->memory, 0, storage->capacity * sizeof(T));
        storage->threadIds = static_cast<PlatformThreadId*>(allocate(&storage->bindings, storage->capacity * sizeof(PlatformThreadId)));
    }

    template<typename T>
    void tls_destruct(ThreadLocalStorage<T>* storage)
    {
        deallocate(&storage->bindings, storage->memory, storage->capacity * sizeof(T));
    }

    template<typename T>
    T* tls_access(ThreadLocalStorage<T>* storage)
    {
        PlatformThreadId currentThreadId = platform_get_current_thread_id();
        for (uSize it = 0; it < storage->size; it++)
        {
            if (storage->threadIds[it] == currentThreadId)
            {
                return &storage->memory[it];
            }
        }
        if (storage->size == storage->capacity)
        {
            uSize newCapacity = storage->capacity * 2;
            T* newMemory = static_cast<T*>(allocate(&storage->bindings, newCapacity * sizeof(T)));
            PlatformThreadId* newThreadIds = static_cast<PlatformThreadId*>(allocate(&storage->bindings, newCapacity * sizeof(PlatformThreadId)));
            std::memcpy(newThreadIds, storage->threadIds, storage->capacity * sizeof(PlatformThreadId));
            std::memcpy(newMemory, storage->memory, storage->capacity * sizeof(T));
            std::memset(newMemory + storage->capacity, 0, (newCapacity - storage->capacity) * sizeof(T));
            deallocate(&storage->bindings, storage->memory, storage->capacity * sizeof(T));
            deallocate(&storage->bindings, storage->threadIds, storage->capacity * sizeof(PlatformThreadId));
            storage->capacity = newCapacity;
            storage->memory = newMemory;
            storage->threadIds = newThreadIds;
        }
        uSize newPosition = storage->size++;
        storage->threadIds[newPosition] = currentThreadId;
        return &storage->memory[newPosition];
    }

    void create_command_pools(RendererBackend* backend, CommandPools* pools)
    {
        auto createPool = [](RendererBackend* backend, u32 queueFamiliIndex) -> VkCommandPool
        {
            VkCommandPool pool;
            VkCommandPoolCreateInfo poolInfo
            {
                .sType              = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                .pNext              = nullptr,
                .flags              = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                .queueFamilyIndex   = queueFamiliIndex,
            };
            al_vk_check(vkCreateCommandPool(backend->gpu.logicalHandle, &poolInfo, &backend->memoryManager.cpu_allocationCallbacks, &pool));
            return pool;
        };
        pools->graphics =
        {
            .handle = createPool(backend, backend->queues.graphics.deviceFamilyIndex),
        };
        // @TODO :  do we need to create separate pool if transfer and graphics queues are in the same family ?
        pools->transfer =
        {
            .handle = createPool(backend, backend->queues.transfer.deviceFamilyIndex),
        };
    }

    void destroy_command_pools(RendererBackend* backend, CommandPools* pools)
    {
        vkDestroyCommandPool(backend->gpu.logicalHandle, pools->graphics.handle, &backend->memoryManager.cpu_allocationCallbacks);
        vkDestroyCommandPool(backend->gpu.logicalHandle, pools->transfer.handle, &backend->memoryManager.cpu_allocationCallbacks);
    }

    CommandPools* get_command_pools(RendererBackend* backend)
    {
        // CommandPools* pools = tls_access(&backend->_commandPools);
        // if (!pools->graphics.handle)
        // {
        //     create_command_pools(backend, pools);
        // }
        // return pools;
        return &backend->_commandPools;
    }

    void create_command_buffers(RendererBackend* backend, CommandBuffers* buffers)
    {
        av_construct(&buffers->primaryBuffers, &backend->memoryManager.cpu_allocationBindings, backend->swapChain.images.count);
        av_construct(&buffers->secondaryBuffers, &backend->memoryManager.cpu_allocationBindings, backend->swapChain.images.count);
        CommandPools* pools = get_command_pools(backend);
        for (uSize it = 0; it < backend->swapChain.images.count; it++)
        {
            // @TODO :  create command buffers for each possible thread
            buffers->primaryBuffers[it] = vk::create_command_buffer(backend->gpu.logicalHandle, pools->graphics.handle, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
            buffers->secondaryBuffers[it] = vk::create_command_buffer(backend->gpu.logicalHandle, pools->graphics.handle, VK_COMMAND_BUFFER_LEVEL_SECONDARY);
        }
        buffers->transferBuffer = vk::create_command_buffer(backend->gpu.logicalHandle, pools->transfer.handle, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    }

    void destroy_command_buffers(RendererBackend* backend, CommandBuffers* buffers)
    {
        av_destruct(buffers->primaryBuffers);
        av_destruct(buffers->secondaryBuffers);
    }

    CommandBuffers* get_command_buffers(RendererBackend* backend)
    {
        // CommandBuffers* buffers = tls_access(&backend->_commandBuffers);
        // if (!buffers->transferBuffer)
        // {
        //     create_command_buffers(backend, buffers);
        // }
        // return buffers;
        return &backend->_commandBuffers;
    }













    // @NOTE :  this method return CommandQueues with valid deviceFamilyIndex values
    Tuple<CommandQueues, VkPhysicalDevice> pick_physical_device(RendererBackend* backend)
    {
        auto isDeviceSuitable = [](RendererBackend* backend, VkPhysicalDevice device) -> Tuple<CommandQueues, bool>
        {
            ArrayView<VkQueueFamilyProperties> familyProperties = vk::get_physical_device_queue_family_properties(device, backend->memoryManager.cpu_allocationBindings);
            defer(av_destruct(familyProperties));
            // @NOTE :  checking if all required queue families are supported by device
            CommandQueues physicalDeviceCommandQueues{ };
            try_pick_graphics_queue(&physicalDeviceCommandQueues.graphics, familyProperties);
            try_pick_present_queue(&physicalDeviceCommandQueues.present, familyProperties, device, backend->surface);
            if (physicalDeviceCommandQueues.graphics.isFamilyPresent)
            {
                try_pick_transfer_queue(&physicalDeviceCommandQueues.transfer, familyProperties, physicalDeviceCommandQueues.graphics.deviceFamilyIndex);
            }
            ArrayView<const char* const> deviceExtensions
            {
                .bindings   = { },
                .memory     = vk::DEVICE_EXTENSIONS,
                .count      = array_size(vk::DEVICE_EXTENSIONS),
            };
            bool isRequiredExtensionsSupported = vk::does_physical_device_supports_required_extensions(device, deviceExtensions, backend->memoryManager.cpu_allocationBindings);
            bool isSwapChainSuppoted = false;
            if (isRequiredExtensionsSupported)
            {
                SwapChainSupportDetails supportDetails = get_swap_chain_support_details(backend->surface, device, backend->memoryManager.cpu_allocationBindings);
                isSwapChainSuppoted = supportDetails.formats.count && supportDetails.presentModes.count;
                av_destruct(supportDetails.formats);
                av_destruct(supportDetails.presentModes);
            }
            return
            {
                physicalDeviceCommandQueues,
                is_command_queues_complete(&physicalDeviceCommandQueues) && isRequiredExtensionsSupported && isSwapChainSuppoted
            };
        };
        ArrayView<VkPhysicalDevice> available = vk::get_available_physical_devices(backend->instance, backend->memoryManager.cpu_allocationBindings);
        defer(av_destruct(available));
        for (uSize it = 0; it < available.count; it++)
        {
            Tuple<CommandQueues, bool> result = isDeviceSuitable(backend, available[it]);
            if (get<1>(result))
            {
                // @TODO :  log device name and shit
                // VkPhysicalDeviceProperties deviceProperties;
                // vkGetPhysicalDeviceProperties(chosenPhysicalDevice, &deviceProperties);
                return
                {
                    get<0>(result),
                    available[it],
                };
            }
        }
        return
        {
            CommandQueues{ },
            VK_NULL_HANDLE
        };
    }

    VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_callback(
                                            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                            VkDebugUtilsMessageTypeFlagsEXT messageType,
                                            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                            void* pUserData) 
    {
        al_vk_log_msg("Debug callback : %s\n", pCallbackData->pMessage);
        return VK_FALSE;
    }
}
