
#include <cassert>
#include <cstdlib>
#include <cstring>

#include "renderer_backend_vulkan.h"

#include "engine/config.h"
#include "engine/utilities/utilities.h"

#define al_vk_log_msg(format, ...)      std::fprintf(stdout, format, __VA_ARGS__);
#define al_vk_log_error(format, ...)    std::fprintf(stderr, format, __VA_ARGS__);

#define al_vk_assert(cmd)                   assert(cmd)
#define al_vk_strcmp(str1, str2)            (::std::strcmp(str1, str2) == 0)
#define al_vk_safe_cast_uSize_u32(value)    static_cast<u32>(value) /*@TODO : implement actual safe cast*/

#define USE_DYNAMIC_STATE

namespace al
{
    template<>
    void renderer_backend_construct<VulkanBackend>(VulkanBackend* backend, RendererBackendInitData* initData)
    {
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
        create_command_pools            (backend);
        create_command_buffers          (backend);
        create_sync_primitives          (backend);
        create_vertex_buffer            (backend);
    }

    template<>
    void renderer_backend_render<VulkanBackend>(VulkanBackend* backend)
    {
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
        VkCommandBuffer commandBuffer = backend->commandBuffers.primaryBuffers[swapChainImageIndex];
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
                VkBuffer vertexBuffers[] = { backend->vertexBuffer };
                VkDeviceSize offsets[] = { 0 };
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
                vkCmdDraw(commandBuffer, static_cast<uint32_t>(array_size(triangle)), 1, 0, 0);
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
    void renderer_backend_destruct<VulkanBackend>(VulkanBackend* backend)
    {
        vkDeviceWaitIdle(backend->gpu.logicalHandle);
        destroy_vertex_buffer       (backend);
        destroy_sync_primitives     (backend);
        destroy_command_buffers     (backend);
        destroy_command_pools       (backend);
        destroy_framebuffers        (backend);
        destroy_render_pipelines    (backend);
        destroy_render_passes       (backend);
        destroy_depth_stencil       (backend);
        destroy_swap_chain          (backend);
        destroy_gpu                 (backend);
        destroy_surface             (backend);
        destroy_instance            (backend);
        destruct                    (&backend->memoryManager);
    }

    template<>
    void renderer_backend_handle_resize<VulkanBackend>(VulkanBackend* backend)
    {
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

    // =================================================================================================================
    // CREATION
    // =================================================================================================================

    void create_instance(VulkanBackend* backend, RendererBackendInitData* initData)
    {
#ifdef AL_DEBUG
        {
            ArrayView<VkLayerProperties> availableValidationLayers = vk::get_available_validation_layers(&backend->memoryManager.cpuAllocationBindings);
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
            ArrayView<VkExtensionProperties> availableInstanceExtensions = vk::get_available_instance_extensions(&backend->memoryManager.cpuAllocationBindings);
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
        al_vk_check(vkCreateInstance(&instanceCreateInfo, &backend->memoryManager.allocationCallbacks, &backend->instance));
#ifdef AL_DEBUG
        backend->debugMessenger = vk::create_debug_messenger(&debugCreateInfo, backend->instance, &backend->memoryManager.allocationCallbacks);
#endif
    }

    void create_surface(VulkanBackend* backend)
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
            al_vk_check(vkCreateWin32SurfaceKHR(backend->instance, &surfaceCreateInfo, &backend->memoryManager.allocationCallbacks, &backend->surface));
        }
#else
#   error Unsupported platform
#endif
    }

    void pick_gpu_and_init_command_queues(VulkanBackend* backend)
    {
        auto getQueueCreateInfos = [](VulkanBackend* backend, CommandQueues* queues) -> ArrayView<VkDeviceQueueCreateInfo>
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
            av_construct(&result, &backend->memoryManager.cpuAllocationBindings, uniqueQueueIndices.count);
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
        VkDevice logicalDevice = VK_NULL_HANDLE;
        if (physicalDevice != VK_NULL_HANDLE)
        {
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
            al_vk_check(vkCreateDevice(physicalDevice, &logicalDeviceCreateInfo, &backend->memoryManager.allocationCallbacks, &logicalDevice));
            for (uSize it = 0; it < CommandQueues::QUEUES_NUM; it++)
            {
                vkGetDeviceQueue(logicalDevice, commandQueues.queues[it].deviceFamilyIndex, 0, &commandQueues.queues[it].handle);
            }
            vk::pick_depth_format(physicalDevice, &backend->depthStencil.format);
        }
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &backend->gpu.memoryProperties);
        backend->gpu.physicalHandle = physicalDevice;
        backend->gpu.logicalHandle = logicalDevice;
        std::memcpy(&backend->queues, &commandQueues, sizeof(CommandQueues));
    }

    void create_swap_chain(VulkanBackend* backend)
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
        auto chooseSwapExtent = [](VulkanBackend* backend, VkSurfaceCapabilitiesKHR* capabilities) -> VkExtent2D
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
            SwapChainSupportDetails supportDetails = get_swap_chain_support_details(backend->surface, backend->gpu.physicalHandle, backend->memoryManager.cpuAllocationBindings);
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
                if (!backend->queues.queues[it].readOnly.isUsedBySwapchain)
                {
                    // @NOTE :  this basically ignores transfer queue
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
            al_vk_check(vkCreateSwapchainKHR(backend->gpu.logicalHandle, &createInfo, &backend->memoryManager.allocationCallbacks, &backend->swapChain.handle));
            al_vk_check(vkGetSwapchainImagesKHR(backend->gpu.logicalHandle, backend->swapChain.handle, &swapChainImageCount, nullptr));
            av_construct(&backend->swapChain.images, &backend->memoryManager.cpuAllocationBindings, swapChainImageCount);
            al_vk_check(vkGetSwapchainImagesKHR(backend->gpu.logicalHandle, backend->swapChain.handle, &swapChainImageCount, backend->swapChain.images.memory));
            backend->swapChain.format = surfaceFormat.format;
            backend->swapChain.extent = extent;
        }
        {
            av_construct(&backend->swapChain.imageViews, &backend->memoryManager.cpuAllocationBindings, backend->swapChain.images.count);
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
                al_vk_check(vkCreateImageView(backend->gpu.logicalHandle, &createInfo, &backend->memoryManager.allocationCallbacks, &backend->swapChain.imageViews[it]));
            }
        }
    }

    void create_depth_stencil(VulkanBackend* backend)
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
        al_vk_check(vkCreateImage(backend->gpu.logicalHandle, &imageCreateInfo, &backend->memoryManager.allocationCallbacks, &backend->depthStencil.image));

        VkMemoryRequirements memoryRequirements;
        vkGetImageMemoryRequirements(backend->gpu.logicalHandle, backend->depthStencil.image, &memoryRequirements);

        VkMemoryAllocateInfo memoryAllocateInfo = { };
        memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memoryAllocateInfo.allocationSize = memoryRequirements.size;
        bool result = vk::get_memory_type_index(&backend->gpu.memoryProperties, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memoryAllocateInfo.memoryTypeIndex);
        al_vk_assert(result);
        al_vk_check(vkAllocateMemory(backend->gpu.logicalHandle, &memoryAllocateInfo, &backend->memoryManager.allocationCallbacks, &backend->depthStencil.memory));
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
        al_vk_check(vkCreateImageView(backend->gpu.logicalHandle, &imageViewCreateInfo, &backend->memoryManager.allocationCallbacks, &backend->depthStencil.view));
    }

    void create_framebuffers(VulkanBackend* backend)
    {
        av_construct(&backend->swapChain.framebuffers, &backend->memoryManager.cpuAllocationBindings, backend->swapChain.images.count);
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
            al_vk_check(vkCreateFramebuffer(backend->gpu.logicalHandle, &framebufferInfo, &backend->memoryManager.allocationCallbacks, &backend->swapChain.framebuffers[it]));
        }
    }

    void create_render_passes(VulkanBackend* backend)
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

        al_vk_check(vkCreateRenderPass(backend->gpu.logicalHandle, &renderPassInfo, &backend->memoryManager.allocationCallbacks, &backend->simpleRenderPass));
    }

    void create_render_pipelines(VulkanBackend* backend)
    {
        PlatformFile vertShader = platform_file_load(backend->memoryManager.cpuAllocationBindings, VulkanBackend::VERTEX_SHADER_PATH, PlatformFileLoadMode::READ);
        PlatformFile fragShader = platform_file_load(backend->memoryManager.cpuAllocationBindings, VulkanBackend::FRAGMENT_SHADER_PATH, PlatformFileLoadMode::READ);
        defer(platform_file_unload(backend->memoryManager.cpuAllocationBindings, vertShader));
        defer(platform_file_unload(backend->memoryManager.cpuAllocationBindings, fragShader));
        VkShaderModule vertShaderModule = vk::create_shader_module(backend->gpu.logicalHandle, { static_cast<u32*>(vertShader.memory), vertShader.sizeBytes }, &backend->memoryManager.allocationCallbacks);
        VkShaderModule fragShaderModule = vk::create_shader_module(backend->gpu.logicalHandle, { static_cast<u32*>(fragShader.memory), fragShader.sizeBytes }, &backend->memoryManager.allocationCallbacks);
        defer(vk::destroy_shader_module(backend->gpu.logicalHandle, vertShaderModule, &backend->memoryManager.allocationCallbacks));
        defer(vk::destroy_shader_module(backend->gpu.logicalHandle, fragShaderModule, &backend->memoryManager.allocationCallbacks));
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
        al_vk_check(vkCreatePipelineLayout(backend->gpu.logicalHandle, &pipelineLayoutInfo, &backend->memoryManager.allocationCallbacks, &backend->pipelineLayout));
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
        al_vk_check(vkCreateGraphicsPipelines(backend->gpu.logicalHandle, VK_NULL_HANDLE, 1, &pipelineInfo, &backend->memoryManager.allocationCallbacks, &backend->pipeline));
    }

    void create_command_pools(VulkanBackend* backend)
    {
        auto createPool = [](VulkanBackend* backend, u32 queueFamiliIndex) -> VkCommandPool
        {
            VkCommandPool pool;
            VkCommandPoolCreateInfo poolInfo
            {
                .sType              = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                .pNext              = nullptr,
                .flags              = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                .queueFamilyIndex   = queueFamiliIndex,
            };
            al_vk_check(vkCreateCommandPool(backend->gpu.logicalHandle, &poolInfo, &backend->memoryManager.allocationCallbacks, &pool));
            return pool;
        };
        backend->commandPools.graphics =
        {
            .handle = createPool(backend, backend->queues.graphics.deviceFamilyIndex),
        };
        // @TODO :  do we need to create separate pool if transfer and graphics queues are in the same family ?
        backend->commandPools.transfer =
        {
            .handle = createPool(backend, backend->queues.transfer.deviceFamilyIndex),
        };
    }

    void create_command_buffers(VulkanBackend* backend)
    {
        av_construct(&backend->commandBuffers.primaryBuffers, &backend->memoryManager.cpuAllocationBindings, backend->swapChain.images.count);
        av_construct(&backend->commandBuffers.secondaryBuffers, &backend->memoryManager.cpuAllocationBindings, backend->swapChain.images.count);
        for (uSize it = 0; it < backend->swapChain.images.count; it++)
        {
            // @TODO :  create command buffers for each possible thread
            backend->commandBuffers.primaryBuffers[it] = vk::create_command_buffer(backend->gpu.logicalHandle, backend->commandPools.graphics.handle, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
            backend->commandBuffers.secondaryBuffers[it] = vk::create_command_buffer(backend->gpu.logicalHandle, backend->commandPools.graphics.handle, VK_COMMAND_BUFFER_LEVEL_SECONDARY);
        }
        backend->commandBuffers.transferBuffer = vk::create_command_buffer(backend->gpu.logicalHandle, backend->commandPools.transfer.handle, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    }

    void create_sync_primitives(VulkanBackend* backend)
    {
        // Semaphores
        av_construct(&backend->syncPrimitives.imageAvailableSemaphores, &backend->memoryManager.cpuAllocationBindings, vk::MAX_IMAGES_IN_FLIGHT);
        av_construct(&backend->syncPrimitives.renderFinishedSemaphores, &backend->memoryManager.cpuAllocationBindings, vk::MAX_IMAGES_IN_FLIGHT);
        for (uSize it = 0; it < vk::MAX_IMAGES_IN_FLIGHT; it++)
        {
            VkSemaphoreCreateInfo semaphoreInfo
            {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                .pNext = nullptr,
                .flags = { },
            };
            al_vk_check(vkCreateSemaphore(backend->gpu.logicalHandle, &semaphoreInfo, &backend->memoryManager.allocationCallbacks, &backend->syncPrimitives.imageAvailableSemaphores[it]));
            al_vk_check(vkCreateSemaphore(backend->gpu.logicalHandle, &semaphoreInfo, &backend->memoryManager.allocationCallbacks, &backend->syncPrimitives.renderFinishedSemaphores[it]));
        }
        // Fences
        av_construct(&backend->syncPrimitives.inFlightFences, &backend->memoryManager.cpuAllocationBindings, vk::MAX_IMAGES_IN_FLIGHT);
        for (uSize it = 0; it < vk::MAX_IMAGES_IN_FLIGHT; it++)
        {
            VkFenceCreateInfo fenceInfo
            {
                .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                .pNext = nullptr,
                .flags = VK_FENCE_CREATE_SIGNALED_BIT,
            };
            al_vk_check(vkCreateFence(backend->gpu.logicalHandle, &fenceInfo, &backend->memoryManager.allocationCallbacks, &backend->syncPrimitives.inFlightFences[it]));
        }
        av_construct(&backend->syncPrimitives.imageInFlightFencesRef, &backend->memoryManager.cpuAllocationBindings, backend->swapChain.images.count);
        for (uSize it = 0; it < backend->syncPrimitives.imageInFlightFencesRef.count; it++)
        {
            backend->syncPrimitives.imageInFlightFencesRef[it] = VK_NULL_HANDLE;
        }
    }

    void create_vertex_buffer(VulkanBackend* backend)
    {
        VkBufferCreateInfo bufferInfo
        {
            .sType                  = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext                  = nullptr,
            .flags                  = 0,
            .size                   = sizeof(triangle),
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
        al_vk_check(vkCreateBuffer(backend->gpu.logicalHandle, &bufferInfo, &backend->memoryManager.allocationCallbacks, &backend->vertexBuffer));
        VkMemoryRequirements memoryRequirements;
        vkGetBufferMemoryRequirements(backend->gpu.logicalHandle, backend->vertexBuffer, &memoryRequirements);
        VkMemoryAllocateInfo memoryAllocateInfo = { };
        memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memoryAllocateInfo.allocationSize = memoryRequirements.size;
        bool result = vk::get_memory_type_index(&backend->gpu.memoryProperties, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memoryAllocateInfo.memoryTypeIndex);
        al_vk_assert(result);
        al_vk_check(vkAllocateMemory(backend->gpu.logicalHandle, &memoryAllocateInfo, &backend->memoryManager.allocationCallbacks, &backend->vertexBufferMemory));
        al_vk_check(vkBindBufferMemory(backend->gpu.logicalHandle, backend->vertexBuffer, backend->vertexBufferMemory, 0));
        {
            VkBuffer stagingBuffer;
            VkDeviceMemory stagingMemory;
            VkBufferCreateInfo stagingBufferInfo
            {
                .sType                  = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .pNext                  = nullptr,
                .flags                  = 0,
                .size                   = sizeof(triangle),
                .usage                  = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                .sharingMode            = VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount  = 0,        // ignored if sharingMode is not VK_SHARING_MODE_CONCURRENT
                .pQueueFamilyIndices    = nullptr,  // ignored if sharingMode is not VK_SHARING_MODE_CONCURRENT
            };
            al_vk_check(vkCreateBuffer(backend->gpu.logicalHandle, &stagingBufferInfo, &backend->memoryManager.allocationCallbacks, &stagingBuffer));
            VkMemoryRequirements memoryRequirements;
            vkGetBufferMemoryRequirements(backend->gpu.logicalHandle, stagingBuffer, &memoryRequirements);
            VkMemoryAllocateInfo memoryAllocateInfo = { };
            memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            memoryAllocateInfo.allocationSize = memoryRequirements.size;
            bool result = vk::get_memory_type_index(&backend->gpu.memoryProperties, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &memoryAllocateInfo.memoryTypeIndex);
            al_vk_assert(result);
            al_vk_check(vkAllocateMemory(backend->gpu.logicalHandle, &memoryAllocateInfo, &backend->memoryManager.allocationCallbacks, &stagingMemory));
            al_vk_check(vkBindBufferMemory(backend->gpu.logicalHandle, stagingBuffer, stagingMemory, 0));
            {
                void* data;
                vkMapMemory(backend->gpu.logicalHandle, stagingMemory, 0, stagingBufferInfo.size, 0, &data);
                std::memcpy(data, triangle, static_cast<uSize>(stagingBufferInfo.size));
                vkUnmapMemory(backend->gpu.logicalHandle, stagingMemory);
            }
            {   // copy buffer
                VkCommandBufferAllocateInfo allocInfo{ };
                allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
                allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
                allocInfo.commandPool = backend->commandPools.transfer.handle;
                allocInfo.commandBufferCount = 1;

                VkCommandBuffer commandBuffer;
                vkAllocateCommandBuffers(backend->gpu.logicalHandle, &allocInfo, &commandBuffer);

                VkCommandBufferBeginInfo beginInfo{ };
                beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

                vkBeginCommandBuffer(commandBuffer, &beginInfo);
                VkBufferCopy copyRegion{};
                copyRegion.size = stagingBufferInfo.size;
                vkCmdCopyBuffer(commandBuffer, stagingBuffer, backend->vertexBuffer, 1, &copyRegion);
                vkEndCommandBuffer(commandBuffer);

                VkSubmitInfo submitInfo{};
                submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers = &commandBuffer;

                vkQueueSubmit(backend->queues.transfer.handle, 1, &submitInfo, VK_NULL_HANDLE);
                vkQueueWaitIdle(backend->queues.transfer.handle);

                vkFreeCommandBuffers(backend->gpu.logicalHandle, backend->commandPools.transfer.handle, 1, &commandBuffer);
            }
            vkDestroyBuffer(backend->gpu.logicalHandle, stagingBuffer, &backend->memoryManager.allocationCallbacks);
            vkFreeMemory(backend->gpu.logicalHandle, stagingMemory, &backend->memoryManager.allocationCallbacks);
        }
    }

    // =================================================================================================================
    // DESTRUCTION
    // =================================================================================================================

    void destroy_instance(VulkanBackend* backend)
    {
        auto DestroyDebugUtilsMessengerEXT = [](VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks* pAllocator)
        {
            PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
            if (func)
            {
                func(instance, messenger, pAllocator);
            }
        };
        DestroyDebugUtilsMessengerEXT(backend->instance, backend->debugMessenger, &backend->memoryManager.allocationCallbacks);
        vkDestroyInstance(backend->instance, &backend->memoryManager.allocationCallbacks);
    }

    void destroy_surface(VulkanBackend* backend)
    {
        vkDestroySurfaceKHR(backend->instance, backend->surface, &backend->memoryManager.allocationCallbacks);
    }

    void destroy_gpu(VulkanBackend* backend)
    {
        vkDestroyDevice(backend->gpu.logicalHandle, &backend->memoryManager.allocationCallbacks);
    }

    void destroy_swap_chain(VulkanBackend* backend)
    {
        for (uSize it = 0; it < backend->swapChain.imageViews.count; it++)
        {
            vkDestroyImageView(backend->gpu.logicalHandle, backend->swapChain.imageViews[it], &backend->memoryManager.allocationCallbacks);
        }
        vkDestroySwapchainKHR(backend->gpu.logicalHandle, backend->swapChain.handle, &backend->memoryManager.allocationCallbacks);
        av_destruct(backend->swapChain.imageViews);
        av_destruct(backend->swapChain.images);
    }

    void destroy_depth_stencil(VulkanBackend* backend)
    {
        vkDestroyImageView(backend->gpu.logicalHandle, backend->depthStencil.view, &backend->memoryManager.allocationCallbacks);
        vkDestroyImage(backend->gpu.logicalHandle, backend->depthStencil.image, &backend->memoryManager.allocationCallbacks);
        vkFreeMemory(backend->gpu.logicalHandle, backend->depthStencil.memory, &backend->memoryManager.allocationCallbacks);
    }

    void destroy_framebuffers(VulkanBackend* backend)
    {
        for (uSize it = 0; it < backend->swapChain.framebuffers.count; it++)
        {
            vkDestroyFramebuffer(backend->gpu.logicalHandle, backend->swapChain.framebuffers[it], &backend->memoryManager.allocationCallbacks);
        }
        av_destruct(backend->swapChain.framebuffers);
    }

    void destroy_render_passes(VulkanBackend* backend)
    {
        vkDestroyRenderPass(backend->gpu.logicalHandle, backend->simpleRenderPass, &backend->memoryManager.allocationCallbacks);
    }

    void destroy_render_pipelines(VulkanBackend* backend)
    {
        vkDestroyPipeline(backend->gpu.logicalHandle, backend->pipeline, &backend->memoryManager.allocationCallbacks);
        vkDestroyPipelineLayout(backend->gpu.logicalHandle, backend->pipelineLayout, &backend->memoryManager.allocationCallbacks);
    }

    void destroy_command_pools(VulkanBackend* backend)
    {
        vkDestroyCommandPool(backend->gpu.logicalHandle, backend->commandPools.graphics.handle, &backend->memoryManager.allocationCallbacks);
        vkDestroyCommandPool(backend->gpu.logicalHandle, backend->commandPools.transfer.handle, &backend->memoryManager.allocationCallbacks);
    }

    void destroy_command_buffers(VulkanBackend* backend)
    {
        av_destruct(backend->commandBuffers.primaryBuffers);
        av_destruct(backend->commandBuffers.secondaryBuffers);
    }

    void destroy_sync_primitives(VulkanBackend* backend)
    {
        // Semaphores
        for (uSize it = 0; it < vk::MAX_IMAGES_IN_FLIGHT; it++)
        {
            vkDestroySemaphore(backend->gpu.logicalHandle, backend->syncPrimitives.imageAvailableSemaphores[it], &backend->memoryManager.allocationCallbacks);
            vkDestroySemaphore(backend->gpu.logicalHandle, backend->syncPrimitives.renderFinishedSemaphores[it], &backend->memoryManager.allocationCallbacks);
        }
        av_destruct(backend->syncPrimitives.imageAvailableSemaphores);
        av_destruct(backend->syncPrimitives.renderFinishedSemaphores);
        // Fences
        for (uSize it = 0; it < vk::MAX_IMAGES_IN_FLIGHT; it++)
        {
            vkDestroyFence(backend->gpu.logicalHandle, backend->syncPrimitives.inFlightFences[it], &backend->memoryManager.allocationCallbacks);
        }
        av_destruct(backend->syncPrimitives.inFlightFences);
        av_destruct(backend->syncPrimitives.imageInFlightFencesRef);
    }

    void destroy_vertex_buffer(VulkanBackend* backend)
    {
        vkDestroyBuffer(backend->gpu.logicalHandle, backend->vertexBuffer, &backend->memoryManager.allocationCallbacks);
        vkFreeMemory(backend->gpu.logicalHandle, backend->vertexBufferMemory, &backend->memoryManager.allocationCallbacks);
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
        memoryManager->cpuAllocationBindings = bindings;
        memoryManager->allocationCallbacks =
        {
            .pUserData = memoryManager,
            .pfnAllocation = 
                [](void* pUserData, uSize size, uSize alignment, VkSystemAllocationScope allocationScope)
                {
                    VulkanMemoryManager* manager = static_cast<VulkanMemoryManager*>(pUserData);
                    void* result = allocate(&manager->cpuAllocationBindings, size);
                    al_vk_assert(manager->currentNumberOfCpuAllocations < VulkanMemoryManager::MAX_CPU_ALLOCATIONS);
                    manager->allocations[manager->currentNumberOfCpuAllocations++] =
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
                    for (uSize it = 0; it < manager->currentNumberOfCpuAllocations; it++)
                    {
                        if (manager->allocations[it].ptr == pOriginal)
                        {
                            deallocate(&manager->cpuAllocationBindings, manager->allocations[it].ptr, manager->allocations[it].size);
                            result = allocate(&manager->cpuAllocationBindings, size);
                            manager->allocations[it] =
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
                    for (uSize it = 0; it < manager->currentNumberOfCpuAllocations; it++)
                    {
                        if (manager->allocations[it].ptr == pMemory)
                        {
                            deallocate(&manager->cpuAllocationBindings, manager->allocations[it].ptr, manager->allocations[it].size);
                            manager->allocations[it] = manager->allocations[manager->currentNumberOfCpuAllocations - 1];
                            manager->currentNumberOfCpuAllocations -= 1;
                            break;
                        }
                    }
                },
            .pfnInternalAllocation  = nullptr,
            .pfnInternalFree        = nullptr
        };
    }

    void destruct(VulkanMemoryManager* memoryManager)
    {
        // @TODO : check if everything is deallocated
    }






    // @NOTE :  this method return CommandQueues with valid deviceFamilyIndex values
    Tuple<CommandQueues, VkPhysicalDevice> pick_physical_device(VulkanBackend* backend)
    {
        auto isDeviceSuitable = [](VulkanBackend* backend, VkPhysicalDevice device) -> Tuple<CommandQueues, bool>
        {
            auto isCommandQueueInfoComplete = [](CommandQueues* queues)
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
            auto doesPhysicalDeviceSupportsRequiredExtensions = [](VulkanBackend* backend, VkPhysicalDevice device, ArrayView<const char* const> extensions) -> bool
            {
                u32 count;
                vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
                ArrayView<VkExtensionProperties> availableExtensions;
                av_construct(&availableExtensions, &backend->memoryManager.cpuAllocationBindings, count);
                defer(av_destruct(availableExtensions));
                vkEnumerateDeviceExtensionProperties(device, nullptr, &count, availableExtensions.memory);
                bool isRequiredExtensionAvailable;
                bool result = true;
                for (uSize requiredIt = 0; requiredIt < extensions.count; requiredIt++)
                {
                    isRequiredExtensionAvailable = false;
                    for (uSize availableIt = 0; availableIt < availableExtensions.count; availableIt++)
                    {
                        if (al_vk_strcmp(availableExtensions[availableIt].extensionName, extensions[requiredIt]))
                        {
                            isRequiredExtensionAvailable = true;
                            break;
                        }
                    }
                    if (!isRequiredExtensionAvailable)
                    {
                        result = false;
                        break;
                    }
                }
                return result;
            };
            CommandQueues physicalDeviceCommandQueues;
            // @NOTE :  getting information about all available queue families in device
            u32 count;
            ArrayView<VkQueueFamilyProperties> familyProperties;
            vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
            av_construct(&familyProperties, &backend->memoryManager.cpuAllocationBindings, count);
            defer(av_destruct(familyProperties));
            vkGetPhysicalDeviceQueueFamilyProperties(device, &count, familyProperties.memory);
            // @NOTE :  checking if all required queue families are supported by device
            VkBool32 isSupported;
            for (uSize it = 0; it < familyProperties.count; it++)
            {
                CommandQueues::SupportQuery query
                {
                    .props       = &familyProperties[it],
                    .surface     = backend->surface,
                    .device      = device,
                    .familyIndex = static_cast<u32>(it)
                };
                for (uSize queueIt = 0; queueIt < CommandQueues::QUEUES_NUM; queueIt++)
                {
                    if (physicalDeviceCommandQueues.queues[queueIt].readOnly.isSupported(&query))
                    {
                        physicalDeviceCommandQueues.queues[queueIt].deviceFamilyIndex = it;
                        physicalDeviceCommandQueues.queues[queueIt].isFamilyPresent = true;
                    }
                }
                if (isCommandQueueInfoComplete(&physicalDeviceCommandQueues))
                {
                    break;
                }
            }
            bool isQueueFamiliesInfoComplete = isCommandQueueInfoComplete(&physicalDeviceCommandQueues);
            ArrayView<const char* const> deviceExtensions
            {
                .bindings   = { },
                .memory     = vk::DEVICE_EXTENSIONS,
                .count      = array_size(vk::DEVICE_EXTENSIONS),
            };
            bool isRequiredExtensionsSupported = doesPhysicalDeviceSupportsRequiredExtensions(backend, device, deviceExtensions);
            bool isSwapChainSuppoted = false;
            if (isRequiredExtensionsSupported)
            {
                SwapChainSupportDetails supportDetails = get_swap_chain_support_details(backend->surface, device, backend->memoryManager.cpuAllocationBindings);
                isSwapChainSuppoted = supportDetails.formats.count && supportDetails.presentModes.count;
                av_destruct(supportDetails.formats);
                av_destruct(supportDetails.presentModes);
            }
            return
            {
                physicalDeviceCommandQueues,
                isQueueFamiliesInfoComplete && isRequiredExtensionsSupported && isSwapChainSuppoted
            };
        };
        ArrayView<VkPhysicalDevice> available = vk::get_available_physical_devices(backend->instance, backend->memoryManager.cpuAllocationBindings);
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








































#if 0

    

    template<typename T>
    T& ArrayView<T>::operator [] (uSize index)
    {
        return memory[index];
    }

    template<typename T>
    uSize vulkan_array_view_memory_size(ArrayView<T> view)
    {
        return view.count * sizeof(T);
    }

    template<typename T>
    void construct(ArrayView<T>* view, AllocatorBindings* bindings, uSize size)
    {
        view->bindings = *bindings;
        view->count = size;
        view->memory = static_cast<T*>(allocate(bindings, size * sizeof(T)));
        std::memset(view->memory, 0, size * sizeof(T));
    }

    template<typename T>
    void destruct(ArrayView<T> view)
    {
        deallocate(&view.bindings, view.memory, view.count * sizeof(T));
    }

    // =================================================================================================================================================
    // =================================================================================================================================================
    // Allocation
    // =================================================================================================================================================
    // =================================================================================================================================================

    void vulkan_set_allocation_callbacks(RendererBackendVulkan* backend)
    {
        // @TODO :  support reallocation in engine/memory allocators, so they can be used as allocation callbacks for vulkan
        backend->vkAllocationCallbacks =
        {
            .pUserData = nullptr,
            .pfnAllocation = 
                [](void* pUserData, uSize size, uSize alignment, VkSystemAllocationScope allocationScope)
                {
                    // log_msg("Allocating %d bytes of memory with alignment of %d bytes. Allocation scope is %s\n", (int)size, (int)alignment, RendererBackendVulkan::ALLOCATION_SCOPE_TO_STR[allocationScope]);
                    void* mem = std::malloc(size); std::memset(mem, 0, size); return mem;
                },
            .pfnReallocation =
                [](void* pUserData, void* pOriginal, uSize size, uSize alignment, VkSystemAllocationScope allocationScope)
                {
                    // log_msg("Reallocating %d bytes of memory with alignment of %d bytes. Allocation scope is %s\n", (int)size, (int)alignment, RendererBackendVulkan::ALLOCATION_SCOPE_TO_STR[allocationScope]);
                    return std::realloc(pOriginal, size);
                },
            .pfnFree =
                [](void* pUserData, void* pMemory)
                {
                    // log_msg("Freeing memory from %p\n", pMemory);
                    std::free(pMemory);
                },
            .pfnInternalAllocation  = nullptr,
            .pfnInternalFree        = nullptr
        };
    }

    // =================================================================================================================================================
    // =================================================================================================================================================
    // Debug
    // =================================================================================================================================================
    // =================================================================================================================================================

    VkDebugUtilsMessengerCreateInfoEXT vulkan_get_debug_messenger_create_info()
    {
        return
        {
            .sType              = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .pNext              = nullptr,
            .flags              = 0,
            .messageSeverity    = /*VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |*/ VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType        = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback    = vulkan_debug_callback,
            .pUserData          = nullptr,
        };
    }

    void vulkan_setup_debug_messenger(RendererBackendVulkan* backend)
    {
        // @NOTE :  adresses of extension functions must be found manually via vkGetInstanceProcAddr
        auto CreateDebugUtilsMessengerEXT = [](VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) -> VkResult
        {
            PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
            if (func)
            {
                return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
            }
            else
            {
                return VK_ERROR_EXTENSION_NOT_PRESENT;
            }
        };
        VkDebugUtilsMessengerCreateInfoEXT createInfo = vulkan_get_debug_messenger_create_info();
        al_vk_check(CreateDebugUtilsMessengerEXT(backend->vkInstance, &createInfo, &backend->vkAllocationCallbacks, &backend->vkDebugMessenger));
    }

    void vulkan_destroy_debug_messenger(RendererBackendVulkan* backend)
    {
        // @NOTE :  adresses of extension functions must be found manually via vkGetInstanceProcAddr
        auto DestroyDebugUtilsMessengerEXT = [](VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks* pAllocator)
        {
            PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
            if (func)
            {
                func(instance, messenger, pAllocator);
            }
        };
        DestroyDebugUtilsMessengerEXT(backend->vkInstance, backend->vkDebugMessenger, &backend->vkAllocationCallbacks);
    }

    // =================================================================================================================================================
    // =================================================================================================================================================
    // VkInstance creation
    // =================================================================================================================================================
    // =================================================================================================================================================

    ArrayView<VkLayerProperties> vulkan_get_available_validation_layers(AllocatorBindings* bindings)
    {
        u32 count;
        vkEnumerateInstanceLayerProperties(&count, nullptr);
        ArrayView<VkLayerProperties> result;
        construct(&result, bindings, count);
        vkEnumerateInstanceLayerProperties(&count, result.memory);
        return result;
    }

    ArrayView<VkExtensionProperties> vulkan_get_available_extensions(AllocatorBindings* bindings)
    {
        u32 count;
        vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
        ArrayView<VkExtensionProperties> result;
        construct(&result, bindings, count);
        vkEnumerateInstanceExtensionProperties(nullptr, &count, result.memory);
        return result;
    }

    void vulkan_create_vk_instance(RendererBackendVulkan* backend, RendererBackendInitData* initData)
    {
        if (initData->isDebug)
        {
            ArrayView<VkLayerProperties> availablevalidationLayers = vulkan_get_available_validation_layers(&backend->bindings);
            defer(destruct(availablevalidationLayers));
            for (uSize requiredIt = 0; requiredIt < RendererBackendVulkan::REQUIRED_VALIDATION_LAYERS_COUNT; requiredIt++)
            {
                const char* requiredLayerName = RendererBackendVulkan::REQUIRED_VALIDATION_LAYERS[requiredIt];
                bool isFound = false;
                for (uSize availableIt = 0; availableIt < availablevalidationLayers.count; availableIt++)
                {
                    const char* availableLayerName = availablevalidationLayers[availableIt].layerName;
                    // @TODO :  replace default strcmp
                    if (std::strcmp(availableLayerName, requiredLayerName) == 0)
                    {
                        isFound = true;
                        break;
                    }
                }
                // @TODO :  replace default assert
                assert(isFound);
            }
        }
        {
            ArrayView<VkExtensionProperties> availableExtensions = vulkan_get_available_extensions(&backend->bindings);
            defer(destruct(availableExtensions));
            for (uSize requiredIt = 0; requiredIt < RendererBackendVulkan::REQUIRED_EXTENSIONS_COUNT; requiredIt++)
            {
                const char* requiredExtensionName = RendererBackendVulkan::REQUIRED_EXTENSIONS[requiredIt];
                bool isFound = false;
                for (uSize availableIt = 0; availableIt < availableExtensions.count; availableIt++)
                {
                    const char* availableExtensionName = availableExtensions[availableIt].extensionName;
                    // @TODO :  replace default strcmp
                    if (std::strcmp(availableExtensionName, requiredExtensionName) == 0)
                    {
                        isFound = true;
                        break;
                    }
                }
                // @TODO :  replace default assert
                assert(isFound);
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
        // @NOTE :  although we setuping debug messenger via __setup_debug_messenger call,
        //          created debug messenger will not be able to recieve information about
        //          vkCreateInstance and vkDestroyInstance calls. So we have to pass additional
        //          VkDebugUtilsMessengerCreateInfoEXT struct pointer to VkInstanceCreateInfo::pNext
        //          to enable debug of these functions
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = vulkan_get_debug_messenger_create_info();
        VkInstanceCreateInfo instanceCreateInfo
        {
            .sType                      = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext                      = &debugCreateInfo,
            .flags                      = 0,
            .pApplicationInfo           = &applicationInfo,
            .enabledLayerCount          = 0,
            .ppEnabledLayerNames        = nullptr,
            .enabledExtensionCount      = RendererBackendVulkan::REQUIRED_EXTENSIONS_COUNT,
            .ppEnabledExtensionNames    = RendererBackendVulkan::REQUIRED_EXTENSIONS,
        };
        if (initData->isDebug)
        {
            instanceCreateInfo.enabledLayerCount    = RendererBackendVulkan::REQUIRED_VALIDATION_LAYERS_COUNT;
            instanceCreateInfo.ppEnabledLayerNames  = RendererBackendVulkan::REQUIRED_VALIDATION_LAYERS;
        }
        al_vk_check(vkCreateInstance(&instanceCreateInfo, &backend->vkAllocationCallbacks, &backend->vkInstance));
    }

    // =================================================================================================================================================
    // =================================================================================================================================================
    // Surface
    // =================================================================================================================================================
    // =================================================================================================================================================

    void vulkan_create_surface(RendererBackendVulkan* backend)
    {
        // @TODO :  move surface creation to platform layer
        // @TODO :  move surface creation to platform layer
        // @TODO :  move surface creation to platform layer
        // @TODO :  move surface creation to platform layer
        // @TODO :  move surface creation to platform layer
        // @TODO :  move surface creation to platform layer
        // @TODO :  move surface creation to platform layer
        // @TODO :  move surface creation to platform layer
        // @TODO :  move surface creation to platform layer
        // @TODO :  move surface creation to platform layer

        VkWin32SurfaceCreateInfoKHR surfaceCreateInfo
        {
            .sType      = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
            .pNext      = nullptr,
            .flags      = 0,
            .hinstance  = ::GetModuleHandle(nullptr),
            .hwnd       = backend->window->handle
        };
        al_vk_check(vkCreateWin32SurfaceKHR(backend->vkInstance, &surfaceCreateInfo, &backend->vkAllocationCallbacks, &backend->vkSurface));
    }

    // =================================================================================================================================================
    // =================================================================================================================================================
    // Physical device queue families info
    // =================================================================================================================================================
    // =================================================================================================================================================

    ArrayView<VkQueueFamilyProperties> vulkan_get_queue_family_properties(RendererBackendVulkan* backend, VkPhysicalDevice device)
    {
        ArrayView<VkQueueFamilyProperties> result;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &result.count, nullptr);
        construct(&result, &backend->bindings, result.count);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &result.count, result.memory);
        return result;
    }

    bool vulkan_is_queue_families_info_complete(VulkanQueueFamiliesInfo* info)
    {
        return  info->familyInfos[VulkanQueueFamiliesInfo::GRAPHICS_FAMILY].isPresent &&
                info->familyInfos[VulkanQueueFamiliesInfo::SURFACE_PRESENT_FAMILY].isPresent;
    }

    VulkanQueueFamiliesInfo vulkan_get_queue_families_info(RendererBackendVulkan* backend, VkPhysicalDevice device)
    {
        VkBool32 isSupported;
        VulkanQueueFamiliesInfo info{ };
        ArrayView<VkQueueFamilyProperties> familyProperties = vulkan_get_queue_family_properties(backend, device);
        defer(destruct(familyProperties));
        for (uSize it = 0; it < familyProperties.count; it++)
        {
            if (familyProperties[it].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                info.familyInfos[VulkanQueueFamiliesInfo::GRAPHICS_FAMILY].index = it;
                info.familyInfos[VulkanQueueFamiliesInfo::GRAPHICS_FAMILY].isPresent = true;
            }
            if (isSupported = false, vkGetPhysicalDeviceSurfaceSupportKHR(device, it, backend->vkSurface, &isSupported), isSupported)
            {
                info.familyInfos[VulkanQueueFamiliesInfo::SURFACE_PRESENT_FAMILY].index = it;
                info.familyInfos[VulkanQueueFamiliesInfo::SURFACE_PRESENT_FAMILY].isPresent = true;
            }
            if (vulkan_is_queue_families_info_complete(&info))
            {
                break;
            }
        }
        return info;
    }

    ArrayView<VkDeviceQueueCreateInfo> vulkan_get_queue_create_infos(RendererBackendVulkan* backend, VulkanQueueFamiliesInfo* queueFamiliesInfo)
    {
        // @NOTE :  this is possible that queue family might support more than one of the required features,
        //          so we have to remove duplicates from queueFamiliesInfo and create VkDeviceQueueCreateInfos
        //          only for unique indexes
        const f32 QUEUE_DEFAULT_PRIORITY = 1.0f;
        bool isFound = false;
        u32 indicesArray[VulkanQueueFamiliesInfo::FAMILIES_NUM];
        ArrayView<u32> uniqueQueueIndices
        {
            .memory = indicesArray,
            .count = 0
        };
        for (uSize it = 0; it < VulkanQueueFamiliesInfo::FAMILIES_NUM; it++)
        {
            isFound = false;
            for (uSize uniqueIt = 0; uniqueIt < uniqueQueueIndices.count; uniqueIt++)
            {
                if (uniqueQueueIndices[uniqueIt] == queueFamiliesInfo->familyInfos[it].index)
                {
                    isFound = true;
                    break;
                }
            }
            if (isFound)
            {
                continue;
            }
            uniqueQueueIndices.count += 1;
            uniqueQueueIndices[uniqueQueueIndices.count - 1] = queueFamiliesInfo->familyInfos[it].index;
        }
        ArrayView<VkDeviceQueueCreateInfo> result;
        construct(&result, &backend->bindings, uniqueQueueIndices.count);
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
    }

    // =================================================================================================================================================
    // =================================================================================================================================================
    // Physical device
    // =================================================================================================================================================
    // =================================================================================================================================================

    bool vulkan_does_physical_device_supports_required_extensions(RendererBackendVulkan* backend, VkPhysicalDevice device)
    {
        // log_msg("Checking required device extensions (not the same as extensions that was before).\n");
        u32 count;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr);
        ArrayView<VkExtensionProperties> availableExtensions
        {
            .memory = static_cast<VkExtensionProperties*>(allocate(&backend->bindings, count * sizeof(VkExtensionProperties))),
            .count = count
        };
        vkEnumerateDeviceExtensionProperties(device, nullptr, &availableExtensions.count, availableExtensions.memory);
        bool isRequiredExtensionAvailable;
        bool result = true;
        for (uSize requiredIt = 0; requiredIt < RendererBackendVulkan::REQUIRED_DEVICE_EXTENSIONS_COUNT; requiredIt++)
        {
            isRequiredExtensionAvailable = false;
            for (uSize availableIt = 0; availableIt < availableExtensions.count; availableIt++)
            {
                if (std::strcmp(availableExtensions[availableIt].extensionName, RendererBackendVulkan::REQUIRED_DEVICE_EXTENSIONS[requiredIt]) == 0)
                {
                    isRequiredExtensionAvailable = true;
                    break;
                }
            }
            if (!isRequiredExtensionAvailable)
            {
                result = false;
                break;
            }
        }
        deallocate(&backend->bindings, availableExtensions.memory, vulkan_array_view_memory_size(availableExtensions));
        return result;
    }

    bool vulkan_is_physical_device_suitable(RendererBackendVulkan* backend, VkPhysicalDevice device)
    {
        VulkanQueueFamiliesInfo queueFamiliesInfo = vulkan_get_queue_families_info(backend, device);
        bool isQueueFamiliesInfoComplete = vulkan_is_queue_families_info_complete(&queueFamiliesInfo);
        bool isRequiredExtensionsSupported = vulkan_does_physical_device_supports_required_extensions(backend, device);
        bool isSwapChainSuppoted = false;
        if (isRequiredExtensionsSupported)
        {
            RendererBackendVulkanSwapChainSupportDetails supportDetails = vulkan_get_swap_chain_support_details(backend->surface, device, backend->memoryManager.cpuAllocationBindings);
            isSwapChainSuppoted = supportDetails.formats.count && supportDetails.presentModes.count;
            deallocate(&backend->bindings, supportDetails.formats.memory, vulkan_array_view_memory_size(supportDetails.formats));
            deallocate(&backend->bindings, supportDetails.presentModes.memory, vulkan_array_view_memory_size(supportDetails.presentModes));
        }
        return isQueueFamiliesInfoComplete && isRequiredExtensionsSupported && isSwapChainSuppoted;
    }

    ArrayView<VkPhysicalDevice> vulkan_get_available_physical_devices(RendererBackendVulkan* backend)
    {
        ArrayView<VkPhysicalDevice> result;
        vkEnumeratePhysicalDevices(backend->vkInstance, &result.count, nullptr);
        construct(&result, &backend->bindings, result.count);
        vkEnumeratePhysicalDevices(backend->vkInstance, &result.count, result.memory);
        return result;
    }

    void vulkan_choose_physical_device(RendererBackendVulkan* backend)
    {
        ArrayView<VkPhysicalDevice> available = vulkan_get_available_physical_devices(backend);
        defer(destruct(available));
        for (uSize it = 0; it < available.count; it++)
        {
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(available[it], &deviceProperties);
            if (vulkan_is_physical_device_suitable(backend, available[it]))
            {
                backend->vkPhysicalDevice = available[it];
                break;
            }
        }
        // @TODO :  replace default assert
        assert(backend->vkPhysicalDevice); // unable to find suitable physical device
    }

    // =================================================================================================================================================
    // =================================================================================================================================================
    // Logical device
    // =================================================================================================================================================
    // =================================================================================================================================================

    void vulkan_create_logical_device(RendererBackendVulkan* backend)
    {
        VulkanQueueFamiliesInfo queueFamiliesInfo = vulkan_get_queue_families_info(backend, backend->vkPhysicalDevice);
        ArrayView<VkDeviceQueueCreateInfo> queueCreateInfos = vulkan_get_queue_create_infos(backend, &queueFamiliesInfo);
        defer(destruct(queueCreateInfos));
        VkPhysicalDeviceFeatures deviceFeatures
        {
        };
        VkDeviceCreateInfo logicalDeviceCreateInfo
        {
            .sType                      = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext                      = nullptr,
            .flags                      = 0,
            .queueCreateInfoCount       = queueCreateInfos.count,
            .pQueueCreateInfos          = queueCreateInfos.memory,
            // @NOTE : Validation layers info is not used by recent vulkan implemetations, but still can be set for compatibility reasons
            .enabledLayerCount          = RendererBackendVulkan::REQUIRED_VALIDATION_LAYERS_COUNT,
            .ppEnabledLayerNames        = RendererBackendVulkan::REQUIRED_VALIDATION_LAYERS,
            .enabledExtensionCount      = RendererBackendVulkan::REQUIRED_DEVICE_EXTENSIONS_COUNT,
            .ppEnabledExtensionNames    = RendererBackendVulkan::REQUIRED_DEVICE_EXTENSIONS,
            .pEnabledFeatures           = &deviceFeatures
        };
        al_vk_check(vkCreateDevice(backend->vkPhysicalDevice, &logicalDeviceCreateInfo, &backend->vkAllocationCallbacks, &backend->vkLogicalDevice));
        vkGetDeviceQueue(backend->vkLogicalDevice, queueFamiliesInfo.familyInfos[VulkanQueueFamiliesInfo::GRAPHICS_FAMILY].index, 0, &backend->vkGraphicsQueue);
        vkGetDeviceQueue(backend->vkLogicalDevice, queueFamiliesInfo.familyInfos[VulkanQueueFamiliesInfo::SURFACE_PRESENT_FAMILY].index, 0, &backend->vkPresentQueue);
    }

    // =================================================================================================================================================
    // =================================================================================================================================================
    // Swap chain
    // =================================================================================================================================================
    // =================================================================================================================================================

    RendererBackendVulkanSwapChainSupportDetails vulkan_get_swap_chain_support_details(RendererBackendVulkan* backend, VkPhysicalDevice device)
    {
        RendererBackendVulkanSwapChainSupportDetails result{ };
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, backend->vkSurface, &result.capabilities);
        u32 formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, backend->vkSurface, &formatCount, nullptr);
        if (formatCount != 0)
        {
            construct(&result.formats, &backend->bindings, formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, backend->vkSurface, &formatCount, result.formats.memory);
        }
        u32 presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, backend->vkSurface, &presentModeCount, nullptr);
        if (presentModeCount != 0)
        {
            construct(&result.presentModes, &backend->bindings, presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, backend->vkSurface, &presentModeCount, result.presentModes.memory);
        }
        return result;
    }

    VkSurfaceFormatKHR vulkan_choose_surface_format(ArrayView<VkSurfaceFormatKHR> availableFormats)
    {
        for (uSize it = 0; it < availableFormats.count; it++)
        {
            if (availableFormats[it].format == VK_FORMAT_B8G8R8A8_SRGB && availableFormats[it].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return availableFormats[it];
            }
        }
        return availableFormats[0];
    }

    VkPresentModeKHR vulkan_choose_presentation_mode(ArrayView<VkPresentModeKHR> availableModes)
    {
        for (uSize it = 0; it < availableModes.count; it++)
        {
            // VK_PRESENT_MODE_MAILBOX_KHR will allow us to implemet triple buffering
            if (availableModes[it] == VK_PRESENT_MODE_MAILBOX_KHR) //VK_PRESENT_MODE_IMMEDIATE_KHR
            {
                return availableModes[it];
            }
        }
        return VK_PRESENT_MODE_FIFO_KHR; // Guarateed to be available
    }

    VkExtent2D vulkan_choose_swap_extent(RendererBackendVulkan* backend, VkSurfaceCapabilitiesKHR* surfaceCapabilities)
    {
        if (surfaceCapabilities->currentExtent.width != UINT32_MAX)
        {
            return surfaceCapabilities->currentExtent;
        }
        else
        {
            auto clamp = [](u32 min, u32 max, u32 value) -> u32
            {
                return value < min ? min : (value > max ? max : value);
            };
            return
            {
                clamp(surfaceCapabilities->minImageExtent.width , surfaceCapabilities->maxImageExtent.width , platform_window_get_current_width (backend->window)),
                clamp(surfaceCapabilities->minImageExtent.height, surfaceCapabilities->maxImageExtent.height, platform_window_get_current_height(backend->window))
            };
        }
    }

    void vulkan_create_swap_chain(RendererBackendVulkan* backend)
    {
        RendererBackendVulkanSwapChainSupportDetails supportDetails = vulkan_get_swap_chain_support_details(backend, backend->vkPhysicalDevice);
        defer(destruct(supportDetails.formats));
        defer(destruct(supportDetails.presentModes));
        VkSurfaceFormatKHR  surfaceFormat   = vulkan_choose_surface_format     (supportDetails.formats);
        VkPresentModeKHR    presentMode     = vulkan_choose_presentation_mode  (supportDetails.presentModes);
        VkExtent2D          extent          = vulkan_choose_swap_extent        (backend, &supportDetails.capabilities);
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
        VulkanQueueFamiliesInfo queueFamiliesInfo = vulkan_get_queue_families_info(backend, backend->vkPhysicalDevice);
        u32 queueFamiliesIndices[] =
        {
            queueFamiliesInfo.familyInfos[VulkanQueueFamiliesInfo::GRAPHICS_FAMILY].index,
            queueFamiliesInfo.familyInfos[VulkanQueueFamiliesInfo::SURFACE_PRESENT_FAMILY].index
        };
        if (queueFamiliesIndices[0] != queueFamiliesIndices[1])
        {
            // @NOTE :  if VK_SHARING_MODE_EXCLUSIVE was used here, we would have to explicitly transfer ownership of swap chain images
            //          from one queue to another
            imageSharingMode        = VK_SHARING_MODE_CONCURRENT;
            queueFamilyIndexCount   = 2;
            pQueueFamilyIndices     = queueFamiliesIndices;
        }
        VkSwapchainCreateInfoKHR createInfo
        {
            .sType                  = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .pNext                  = nullptr,
            .flags                  = 0,
            .surface                = backend->vkSurface,
            .minImageCount          = imageCount,
            .format            = surfaceFormat.format,
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
        al_vk_check(vkCreateSwapchainKHR(backend->vkLogicalDevice, &createInfo, &backend->vkAllocationCallbacks, &backend->vkSwapChain));
        vkGetSwapchainImagesKHR(backend->vkLogicalDevice, backend->vkSwapChain, &backend->vkSwapChainImages.count, nullptr);
        construct(&backend->vkSwapChainImages, &backend->bindings, backend->vkSwapChainImages.count);
        vkGetSwapchainImagesKHR(backend->vkLogicalDevice, backend->vkSwapChain, &backend->vkSwapChainImages.count, backend->vkSwapChainImages.memory);
        backend->vkSwapChainImageFormat = surfaceFormat.format;
        backend->vkSwapChainExtent = extent;
    }

    void vulkan_destroy_swap_chain(RendererBackendVulkan* backend)
    {
        vkDestroySwapchainKHR(backend->vkLogicalDevice, backend->vkSwapChain, &backend->vkAllocationCallbacks);
        destruct(backend->vkSwapChainImages);
    }

    void vulkan_create_swap_chain_image_views(RendererBackendVulkan* backend)
    {
        construct(&backend->vkSwapChainImageViews, &backend->bindings, backend->vkSwapChainImages.count);
        for (uSize it = 0; it < backend->vkSwapChainImageViews.count; it++)
        {
            VkImageViewCreateInfo createInfo
            {
                .sType              = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .pNext              = nullptr,
                .flags              = 0,
                .image              = backend->vkSwapChainImages[it],
                .viewType           = VK_IMAGE_VIEW_TYPE_2D,
                .format             = backend->vkSwapChainImageFormat,
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
            al_vk_check(vkCreateImageView(backend->vkLogicalDevice, &createInfo, &backend->vkAllocationCallbacks, &backend->vkSwapChainImageViews[it]));
        }
    }

    void vulkan_destroy_swap_chain_image_views(RendererBackendVulkan* backend)
    {
        for (uSize it = 0; it < backend->vkSwapChainImageViews.count; it++)
        {
            vkDestroyImageView(backend->vkLogicalDevice, backend->vkSwapChainImageViews[it], &backend->vkAllocationCallbacks);
        }
        destruct(backend->vkSwapChainImageViews);
    }

    // =================================================================================================================================================
    // =================================================================================================================================================
    // Render pass
    // =================================================================================================================================================
    // =================================================================================================================================================

    VkShaderModule vulkan_create_shader_module(RendererBackendVulkan* backend, PlatformFile spirvBytecode)
    {
        VkShaderModuleCreateInfo createInfo
        {
            .sType      = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .pNext      = nullptr,
            .flags      = 0,
            .codeSize   = spirvBytecode.sizeBytes,
            .pCode      = static_cast<u32*>(spirvBytecode.memory),
        };
        VkShaderModule shaderModule{ };
        al_vk_check(vkCreateShaderModule(backend->vkLogicalDevice, &createInfo, &backend->vkAllocationCallbacks, &shaderModule));
        return shaderModule;
    }

    void vulkan_create_render_pass(RendererBackendVulkan* backend)
    {
        VkAttachmentDescription attachments[] = 
        {
            {   // Color attachment
                .flags          = 0,
                .format         = backend->vkSwapChainImageFormat,
                .samples        = VK_SAMPLE_COUNT_1_BIT,
                .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
                .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            }
        };
        VkAttachmentReference attachemntRefs[] =
        {
            {
                .attachment = 0, // This is the index into the VkAttachmentDescription array
                .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            }
        };
        VkSubpassDescription subpasses[] = 
        {
            {
                .flags                      = { },
                .pipelineBindPoint          = VK_PIPELINE_BIND_POINT_GRAPHICS,
                .inputAttachmentCount       = 0,
                .pInputAttachments          = nullptr,
                .colorAttachmentCount       = sizeof(attachemntRefs) / sizeof(attachemntRefs[0]),
                .pColorAttachments          = attachemntRefs,
                .pResolveAttachments        = nullptr,
                .pDepthStencilAttachment    = nullptr,
                .preserveAttachmentCount    = 0,
                .pPreserveAttachments       = nullptr,
            }
        };
        VkSubpassDependency dependencies[] = 
        {
            {
                .srcSubpass         = VK_SUBPASS_EXTERNAL,
                .dstSubpass         = 0,
                .srcStageMask       = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                .dstStageMask       = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                .srcAccessMask      = 0,
                .dstAccessMask      = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                .dependencyFlags    = 0,
            }
        };
        VkRenderPassCreateInfo renderPassInfo
        {
            .sType              = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .pNext              = nullptr,
            .flags              = 0,
            .attachmentCount    = sizeof(attachments) / sizeof(attachments[0]),
            .pAttachments       = attachments,
            .subpassCount       = sizeof(subpasses) / sizeof(subpasses[0]),
            .pSubpasses         = subpasses,
            .dependencyCount    = sizeof(dependencies) / sizeof(dependencies[0]),
            .pDependencies      = dependencies,
        };
        al_vk_check(vkCreateRenderPass(backend->vkLogicalDevice, &renderPassInfo, &backend->vkAllocationCallbacks, &backend->vkRenderPass));
    }

    // =================================================================================================================================================
    // =================================================================================================================================================
    // Descriptor sets
    // =================================================================================================================================================
    // =================================================================================================================================================

    void vulkan_create_discriptor_set_layout (RendererBackendVulkan* backend)
    {
        VkDescriptorSetLayoutBinding vertexBufferLayoutBinding
        {
            .binding            = 0, // Corresponds to binding in the shader (layout(binding = 0))
            .descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, // Is this correct ? 
            .descriptorCount    = 1,
            .stageFlags         = VK_SHADER_STAGE_VERTEX_BIT,
            .pImmutableSamplers = nullptr,
        };
        VkDescriptorSetLayoutCreateInfo layoutInfo
        {
            .sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext          = nullptr,
            .flags          = 0,
            .bindingCount   = 1,
            .pBindings      = &vertexBufferLayoutBinding,
        };
        al_vk_check(vkCreateDescriptorSetLayout(backend->vkLogicalDevice, &layoutInfo, &backend->vkAllocationCallbacks, &backend->vkDescriptorSetLayout));
    }

    // =================================================================================================================================================
    // =================================================================================================================================================
    // Render pipeline
    // =================================================================================================================================================
    // =================================================================================================================================================

    void vulkan_create_render_pipeline(RendererBackendVulkan* backend)
    {
        PlatformFile vertShader = platform_file_load(backend->bindings, RendererBackendVulkan::VERTEX_SHADER_PATH, PlatformFileLoadMode::READ);
        PlatformFile fragShader = platform_file_load(backend->bindings, RendererBackendVulkan::FRAGMENT_SHADER_PATH, PlatformFileLoadMode::READ);
        defer(platform_file_unload(backend->bindings, vertShader));
        defer(platform_file_unload(backend->bindings, fragShader));
        VkShaderModule vertShaderModule = vulkan_create_shader_module(backend, vertShader);
        VkShaderModule fragShaderModule = vulkan_create_shader_module(backend, fragShader);
        defer(vkDestroyShaderModule(backend->vkLogicalDevice, vertShaderModule, &backend->vkAllocationCallbacks));
        defer(vkDestroyShaderModule(backend->vkLogicalDevice, fragShaderModule, &backend->vkAllocationCallbacks));
        VkPipelineShaderStageCreateInfo shaderStages[] =
        {
            {
                .sType                  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext                  = nullptr,
                .flags                  = 0,
                .stage                  = VK_SHADER_STAGE_VERTEX_BIT,
                .module                 = vertShaderModule,
                .pName                  = "main",   // program entrypoint
                .pSpecializationInfo    = nullptr   // values for shader constants
            },
            {
                .sType                  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext                  = nullptr,
                .flags                  = 0,
                .stage                  = VK_SHADER_STAGE_FRAGMENT_BIT,
                .module                 = fragShaderModule,
                .pName                  = "main",   // program entrypoint
                .pSpecializationInfo    = nullptr   // values for shader constants
            }
        };
        VkPipelineVertexInputStateCreateInfo vertexInputInfo
        {
            .sType                              = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .pNext                              = nullptr,
            .flags                              = 0,
            .vertexBindingDescriptionCount      = 0,
            .pVertexBindingDescriptions         = nullptr,
            .vertexAttributeDescriptionCount    = 0,
            .pVertexAttributeDescriptions       = nullptr,
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
            .width      = static_cast<float>(backend->vkSwapChainExtent.width),
            .height     = static_cast<float>(backend->vkSwapChainExtent.height),
            .minDepth   = 0.0f,
            .maxDepth   = 1.0f,
        };
        VkRect2D scissor
        {
            .offset = { 0, 0 },
            .extent = backend->vkSwapChainExtent,
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
        // VkPipelineDepthStencilStateCreateInfo{ };
        VkPipelineColorBlendAttachmentState colorBlendAttachment
        {
            .blendEnable            = VK_FALSE,
            .srcColorBlendFactor    = VK_BLEND_FACTOR_ONE,  // optional if blendEnable == VK_FALSE
            .dstColorBlendFactor    = VK_BLEND_FACTOR_ZERO, // optional if blendEnable == VK_FALSE
            .colorBlendOp           = VK_BLEND_OP_ADD,      // optional if blendEnable == VK_FALSE
            .srcAlphaBlendFactor    = VK_BLEND_FACTOR_ONE,  // optional if blendEnable == VK_FALSE
            .dstAlphaBlendFactor    = VK_BLEND_FACTOR_ZERO, // optional if blendEnable == VK_FALSE
            .alphaBlendOp           = VK_BLEND_OP_ADD,      // optional if blendEnable == VK_FALSE
            .colorWriteMask         = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        };
        VkPipelineColorBlendStateCreateInfo colorBlending
        {
            .sType              = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .pNext              = nullptr,
            .flags              = 0,
            .logicOpEnable      = VK_FALSE,
            .logicOp            = VK_LOGIC_OP_COPY, // optional if logicOpEnable == VK_FALSE
            .attachmentCount    = 1,
            .pAttachments       = &colorBlendAttachment,
            .blendConstants     = { 0.0f, 0.0f, 0.0f, 0.0f }, // optional (???)
        };
#ifdef USE_DYNAMIC_STATE
        // @NOTE :  Example of dynamic state settings (this parameters of pipeline
        //          will be able to be changed at runtime without recreating the pipeline)
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
#endif
        VkPipelineLayoutCreateInfo pipelineLayoutInfo
        {
            .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext                  = nullptr,
            .flags                  = 0,
            .setLayoutCount         = 1,
            .pSetLayouts            = &backend->vkDescriptorSetLayout,
            .pushConstantRangeCount = 0,
            .pPushConstantRanges    = nullptr,
        };
        al_vk_check(vkCreatePipelineLayout(backend->vkLogicalDevice, &pipelineLayoutInfo, &backend->vkAllocationCallbacks, &backend->vkPipelineLayout));
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
            .pDepthStencilState     = nullptr,
            .pColorBlendState       = &colorBlending,
#ifdef USE_DYNAMIC_STATE
            .pDynamicState          = &dynamicState,
#else
            .pDynamicState          = nullptr,
#endif
            .layout                 = backend->vkPipelineLayout,
            .renderPass             = backend->vkRenderPass,
            .subpass                = 0,
            .basePipelineHandle     = VK_NULL_HANDLE,
            .basePipelineIndex      = -1,
        };
        al_vk_check(vkCreateGraphicsPipelines(backend->vkLogicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, &backend->vkAllocationCallbacks, &backend->vkGraphicsPipeline));
    }

    // =================================================================================================================================================
    // =================================================================================================================================================
    // Framebuffers
    // =================================================================================================================================================
    // =================================================================================================================================================

    void vulkan_create_framebuffers(RendererBackendVulkan* backend)
    {
        construct(&backend->vkFramebuffers, &backend->bindings, backend->vkSwapChainImageViews.count);
        for (uSize it = 0; it < backend->vkSwapChainImageViews.count; it++)
        {
            VkImageView attachments[] =
            {
                backend->vkSwapChainImageViews[it]
            };
            VkFramebufferCreateInfo framebufferInfo
            {
                .sType              = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .pNext              = nullptr,
                .flags              = 0,
                .renderPass         = backend->vkRenderPass,
                .attachmentCount    = sizeof(attachments) / sizeof(attachments[0]),
                .pAttachments       = attachments,
                .width              = backend->vkSwapChainExtent.width,
                .height             = backend->vkSwapChainExtent.height,
                .layers             = 1,
            };
            al_vk_check(vkCreateFramebuffer(backend->vkLogicalDevice, &framebufferInfo, &backend->vkAllocationCallbacks, &backend->vkFramebuffers[it]));
        }
    }

    void vulkan_destroy_framebuffers(RendererBackendVulkan* backend)
    {
        for (uSize it = 0; it < backend->vkFramebuffers.count; it++)
        {
            vkDestroyFramebuffer(backend->vkLogicalDevice, backend->vkFramebuffers[it], &backend->vkAllocationCallbacks);
        }
        destruct(backend->vkFramebuffers);
    }

    // =================================================================================================================================================
    // =================================================================================================================================================
    // Command pools and command buffers
    // =================================================================================================================================================
    // =================================================================================================================================================

    void vulkan_create_command_pool(RendererBackendVulkan* backend)
    {
        VulkanQueueFamiliesInfo queueFamiliesInfo = vulkan_get_queue_families_info(backend, backend->vkPhysicalDevice);
        VkCommandPoolCreateInfo poolInfo
        {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext              = nullptr,
#ifdef USE_DYNAMIC_STATE
            .flags              = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
#else
            .flags              = 0,
#endif
            .queueFamilyIndex   = queueFamiliesInfo.familyInfos[VulkanQueueFamiliesInfo::GRAPHICS_FAMILY].index
        };
        al_vk_check(vkCreateCommandPool(backend->vkLogicalDevice, &poolInfo, &backend->vkAllocationCallbacks, &backend->vkCommandPool));

    }

    void vulkan_destroy_command_pool(RendererBackendVulkan* backend)
    {
        vkDestroyCommandPool(backend->vkLogicalDevice, backend->vkCommandPool, &backend->vkAllocationCallbacks);
    }

    void vulkan_create_command_buffers(RendererBackendVulkan* backend)
    {
#ifdef USE_DYNAMIC_STATE
        construct(&backend->vkCommandBuffers, &backend->bindings, 1);
        VkCommandBufferAllocateInfo allocInfo
        {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext              = nullptr,
            .commandPool        = backend->vkCommandPool,
            .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            // @TODO :  safe-cast from size_t to u32
            .commandBufferCount = static_cast<u32>(backend->vkCommandBuffers.count),
        };
        al_vk_check(vkAllocateCommandBuffers(backend->vkLogicalDevice, &allocInfo, backend->vkCommandBuffers.memory));
#else
        // @NOTE :  creating command buffers for each possible swap chain image
        construct(&backend->vkCommandBuffers, &backend->bindings, backend->vkSwapChainImageViews.count);
        VkCommandBufferAllocateInfo allocInfo
        {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext              = nullptr,
            .commandPool        = backend->vkCommandPool,
            .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            // @TODO :  safe-cast from size_t to u32
            .commandBufferCount = static_cast<u32>(backend->vkCommandBuffers.count),
        };
        al_vk_check(vkAllocateCommandBuffers(backend->vkLogicalDevice, &allocInfo, backend->vkCommandBuffers.memory));
        // @NOTE :  pre-recording command buffer for drawing a triangle
        for (uSize it = 0; it < backend->vkCommandBuffers.count; it++) {
            VkCommandBuffer commandBuffer = backend->vkCommandBuffers[it];
            VkCommandBufferBeginInfo beginInfo
            {
                .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .pNext              = nullptr,
                .flags              = { },
                .pInheritanceInfo   = nullptr,  // relevant only for secondary command buffers
            };
            al_vk_check(vkBeginCommandBuffer(commandBuffer, &beginInfo));
            VkClearValue clearValue
            {
                .color = { 0.1f, 0.1f, 0.1f, 1.0f }
            };
            VkRenderPassBeginInfo renderPassInfo
            {
                .sType              = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                .pNext              = nullptr,
                .renderPass         = backend->vkRenderPass,
                .framebuffer        = backend->vkFramebuffers[it],
                .renderArea         = { .offset = { 0, 0 }, .extent = backend->vkSwapChainExtent },
                .clearValueCount    = 1,
                .pClearValues       = &clearValue,
            };
            vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, backend->vkGraphicsPipeline);
            vkCmdDraw(commandBuffer, 3, 1, 0, 0);
            vkCmdEndRenderPass(commandBuffer);
            al_vk_check(vkEndCommandBuffer(commandBuffer));
        }
#endif
    }

    void vulkan_destroy_command_buffers(RendererBackendVulkan* backend)
    {
        destruct(backend->vkCommandBuffers);
    }

    // =================================================================================================================================================
    // =================================================================================================================================================
    // Synchronization primitives
    // =================================================================================================================================================
    // =================================================================================================================================================

    void vulkan_create_semaphores(RendererBackendVulkan* backend)
    {
        construct(&backend->vkImageAvailableSemaphores, &backend->bindings, RendererBackendVulkan::MAX_IMAGES_IN_FLIGHT);
        construct(&backend->vkRenderFinishedSemaphores, &backend->bindings, RendererBackendVulkan::MAX_IMAGES_IN_FLIGHT);
        for (uSize it = 0; it < RendererBackendVulkan::MAX_IMAGES_IN_FLIGHT; it++)
        {
            VkSemaphoreCreateInfo semaphoreInfo
            {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                .pNext = nullptr,
                .flags = { },
            };
            al_vk_check(vkCreateSemaphore(backend->vkLogicalDevice, &semaphoreInfo, &backend->vkAllocationCallbacks, &backend->vkImageAvailableSemaphores[it]));
            al_vk_check(vkCreateSemaphore(backend->vkLogicalDevice, &semaphoreInfo, &backend->vkAllocationCallbacks, &backend->vkRenderFinishedSemaphores[it]));
        }
    }

    void vulkan_destroy_semaphores(RendererBackendVulkan* backend)
    {
        for (uSize it = 0; it < RendererBackendVulkan::MAX_IMAGES_IN_FLIGHT; it++)
        {
            vkDestroySemaphore(backend->vkLogicalDevice, backend->vkImageAvailableSemaphores[it], &backend->vkAllocationCallbacks);
            vkDestroySemaphore(backend->vkLogicalDevice, backend->vkRenderFinishedSemaphores[it], &backend->vkAllocationCallbacks);
        }
        destruct(backend->vkImageAvailableSemaphores);
        destruct(backend->vkRenderFinishedSemaphores);
    }

    void vulkan_create_fences(RendererBackendVulkan* backend)
    {
        construct(&backend->vkInFlightFences, &backend->bindings, RendererBackendVulkan::MAX_IMAGES_IN_FLIGHT);
        for (uSize it = 0; it < RendererBackendVulkan::MAX_IMAGES_IN_FLIGHT; it++)
        {
            VkFenceCreateInfo fenceInfo
            {
                .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                .pNext = nullptr,
                .flags = VK_FENCE_CREATE_SIGNALED_BIT,
            };
            al_vk_check(vkCreateFence(backend->vkLogicalDevice, &fenceInfo, &backend->vkAllocationCallbacks, &backend->vkInFlightFences[it]));
        }
        construct(&backend->vkImageInFlightFences, &backend->bindings, backend->vkSwapChainImages.count);
        for (uSize it = 0; it < backend->vkImageInFlightFences.count; it++)
        {
            backend->vkImageInFlightFences[it] = VK_NULL_HANDLE;
        }
    }

    void vulkan_destroy_fences(RendererBackendVulkan* backend)
    {
        for (uSize it = 0; it < RendererBackendVulkan::MAX_IMAGES_IN_FLIGHT; it++)
        {
            // @NOTE :  fences from vkImageInFlightFences doesn't need to be destroyed because they reference already destroyed fences from vkInFlightFences
            vkDestroyFence(backend->vkLogicalDevice, backend->vkInFlightFences[it], &backend->vkAllocationCallbacks);
        }
        destruct(backend->vkInFlightFences);
        destruct(backend->vkImageInFlightFences);
    }

    // =================================================================================================================================================
    // =================================================================================================================================================
    // Swap chain recreation logic
    // =================================================================================================================================================
    // =================================================================================================================================================

    void vulkan_cleanup_swap_chain(RendererBackendVulkan* backend)
    {
        for (uSize it = 0; it < backend->vkFramebuffers.count; it++)
        {
            vkDestroyFramebuffer(backend->vkLogicalDevice, backend->vkFramebuffers[it], &backend->vkAllocationCallbacks);
        }
        // @TODO :  safe cast
        vkFreeCommandBuffers    (backend->vkLogicalDevice, backend->vkCommandPool, static_cast<u32>(backend->vkCommandBuffers.count), backend->vkCommandBuffers.memory);
#ifndef USE_DYNAMIC_STATE
        vkDestroyPipeline       (backend->vkLogicalDevice, backend->vkGraphicsPipeline, &backend->vkAllocationCallbacks);
        vkDestroyPipelineLayout (backend->vkLogicalDevice, backend->vkPipelineLayout, &backend->vkAllocationCallbacks);
#endif
        vkDestroyRenderPass     (backend->vkLogicalDevice, backend->vkRenderPass, &backend->vkAllocationCallbacks);
        for (uSize it = 0; it < backend->vkSwapChainImageViews.count; it++)
        {
            vkDestroyImageView(backend->vkLogicalDevice, backend->vkSwapChainImageViews[it], &backend->vkAllocationCallbacks);
        }
        vkDestroySwapchainKHR(backend->vkLogicalDevice, backend->vkSwapChain, &backend->vkAllocationCallbacks);
    }

    void vulkan_recreate_swap_chain(RendererBackendVulkan* backend)
    {
        vkDeviceWaitIdle(backend->vkLogicalDevice);
        vulkan_cleanup_swap_chain           (backend);
        vulkan_create_swap_chain            (backend);
        vulkan_create_swap_chain_image_views(backend);
        vulkan_create_render_pass           (backend);
#ifndef USE_DYNAMIC_STATE
        vulkan_create_render_pipeline       (backend);
#endif
        vulkan_create_framebuffers          (backend);
        vulkan_create_command_buffers       (backend);
    }

    // =================================================================================================================================================
    // =================================================================================================================================================
    // Renderer backend interface
    // =================================================================================================================================================
    // =================================================================================================================================================

    template<>
    void renderer_backend_construct<RendererBackendVulkan>(RendererBackendVulkan* backend, RendererBackendInitData* initData)
    {
        // Shitty test vulkan implementation

        log_msg("Constructing vulkan backend\n");

        backend->bindings = initData->bindings;
        backend->window = initData->window;
        vulkan_set_allocation_callbacks         (backend);
        vulkan_create_vk_instance               (backend, initData);
        vulkan_setup_debug_messenger            (backend);
        vulkan_create_surface                   (backend);
        vulkan_choose_physical_device           (backend);
        vulkan_create_logical_device            (backend);
        vulkan_create_swap_chain                (backend);
        vulkan_create_swap_chain_image_views    (backend);
        vulkan_create_render_pass               (backend);
        vulkan_create_discriptor_set_layout     (backend);
        vulkan_create_render_pipeline           (backend);
        vulkan_create_framebuffers              (backend);
        vulkan_create_command_pool              (backend);
        vulkan_create_command_buffers           (backend);
        vulkan_create_semaphores                (backend);
        vulkan_create_fences                    (backend);
    }

    template<>
    void renderer_backend_render<RendererBackendVulkan>(RendererBackendVulkan* backend)
    {
        uSize currentFrame = backend->currentFrameNumber % RendererBackendVulkan::MAX_IMAGES_IN_FLIGHT;
        backend->currentFrameNumber = currentFrame;

        // @NOTE :  wait for fence of current frame
        vkWaitForFences(backend->vkLogicalDevice, 1, &backend->vkInFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        u32 imageIndex;
        // @NOTE :  timeout value is in nanoseconds. Using UINT64_MAX disables timeout
        VkResult acquireResult = vkAcquireNextImageKHR(backend->vkLogicalDevice, backend->vkSwapChain, UINT64_MAX, backend->vkImageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
        if (acquireResult != VK_SUCCESS)
        {
            // @TODO :  handle function result (https://khronos.org/registry/vulkan/specs/1.2-extensions/man/html/vkAcquireNextImageKHR.html)
            assert(false);
        }

        // @NOTE :  must check for current swap chain image fence (commands for this image might still be executing at this point -
        //          this can happen only if RendererBackendVulkan::MAX_IMAGES_IN_FLIGHT is bigger than number of swap chain images)
        if (backend->vkImageInFlightFences[imageIndex] != VK_NULL_HANDLE)
        {
            vkWaitForFences(backend->vkLogicalDevice, 1, &backend->vkImageInFlightFences[imageIndex], VK_TRUE, UINT64_MAX);
        }
        // @NOTE :  this marks this image as occupied by this frame
        backend->vkImageInFlightFences[imageIndex] = backend->vkInFlightFences[currentFrame];

        VkSemaphore waitSemaphores[] =
        {
            backend->vkImageAvailableSemaphores[currentFrame]
        };
        VkPipelineStageFlags waitStages[] =
        {
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        };
        VkSemaphore signalSemaphores[] =
        {
            backend->vkRenderFinishedSemaphores[currentFrame]
        };
#ifdef USE_DYNAMIC_STATE
        VkCommandBuffer commandBuffer = backend->vkCommandBuffers[0];
        VkCommandBufferBeginInfo beginInfo
        {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext              = nullptr,
            .flags              = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            .pInheritanceInfo   = nullptr,  // relevant only for secondary command buffers
        };
        al_vk_check(vkBeginCommandBuffer(commandBuffer, &beginInfo));
        VkClearValue clearValue
        {
            .color = { 0.1f, 0.1f, 0.1f, 1.0f }
        };
        VkRenderPassBeginInfo renderPassInfo
        {
            .sType              = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .pNext              = nullptr,
            .renderPass         = backend->vkRenderPass,
            .framebuffer        = backend->vkFramebuffers[imageIndex],
            .renderArea         = { .offset = { 0, 0 }, .extent = backend->vkSwapChainExtent },
            .clearValueCount    = 1,
            .pClearValues       = &clearValue,
        };
        VkViewport viewport
        {
            .x          = 0.0f,
            .y          = 0.0f,
            .width      = static_cast<float>(backend->vkSwapChainExtent.width),
            .height     = static_cast<float>(backend->vkSwapChainExtent.height),
            .minDepth   = 0.0f,
            .maxDepth   = 1.0f,
        };
        VkRect2D scissor
        {
            .offset = { 0, 0 },
            .extent = backend->vkSwapChainExtent,
        };
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, backend->vkGraphicsPipeline);
        vkCmdDraw(commandBuffer, 3, 1, 0, 0);
        vkCmdEndRenderPass(commandBuffer);
        al_vk_check(vkEndCommandBuffer(commandBuffer));
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
        vkResetFences(backend->vkLogicalDevice, 1, &backend->vkInFlightFences[currentFrame]);
        vkQueueSubmit(backend->vkGraphicsQueue, 1, &submitInfo, backend->vkInFlightFences[currentFrame]);
#else
        VkSubmitInfo submitInfo
        {
            .sType                  = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext                  = nullptr,
            .waitSemaphoreCount     = sizeof(waitSemaphores) / sizeof(waitSemaphores[0]),
            .pWaitSemaphores        = waitSemaphores,
            .pWaitDstStageMask      = waitStages,
            .commandBufferCount     = 1,
            .pCommandBuffers        = &backend->vkCommandBuffers[imageIndex],
            .signalSemaphoreCount   = sizeof(signalSemaphores) / sizeof(signalSemaphores[0]),
            .pSignalSemaphores      = signalSemaphores,
        };
        vkResetFences(backend->vkLogicalDevice, 1, &backend->vkInFlightFences[currentFrame]);
        vkQueueSubmit(backend->vkGraphicsQueue, 1, &submitInfo, backend->vkInFlightFences[currentFrame]);
#endif
        VkSwapchainKHR swapChains[] =
        {
            backend->vkSwapChain
        };
        VkPresentInfoKHR presentInfo
        {
            .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pNext              = nullptr,
            .waitSemaphoreCount = sizeof(signalSemaphores) / sizeof(signalSemaphores[0]),
            .pWaitSemaphores    = signalSemaphores,
            .swapchainCount     = sizeof(swapChains) / sizeof(swapChains[0]),
            .pSwapchains        = swapChains,
            .pImageIndices      = &imageIndex,
            .pResults           = nullptr,
        };
        al_vk_check(vkQueuePresentKHR(backend->vkPresentQueue, &presentInfo));
#ifdef USE_DYNAMIC_STATE
        // @TODO :  if VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT flag in VkCommandPoolCreateInfo is true,
        //          command buffer gets implicitly reset when calling vkBeginCommandBuffer
        //          https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkCommandPoolCreateFlagBits.html
        // vkQueueWaitIdle(backend->vkPresentQueue);
        // al_vk_check(vkResetCommandBuffer(commandBuffer, 0));
#endif
    }

    template<>
    void renderer_backend_destruct<RendererBackendVulkan>(RendererBackendVulkan* backend)
    {
        log_msg("Destructing vulkan backend\n");

        // @NOTE :  wait for all commands to finish execution
        vkDeviceWaitIdle(backend->vkLogicalDevice);

#ifdef USE_DYNAMIC_STATE
        vkDestroyPipeline       (backend->vkLogicalDevice, backend->vkGraphicsPipeline, &backend->vkAllocationCallbacks);
        vkDestroyPipelineLayout (backend->vkLogicalDevice, backend->vkPipelineLayout, &backend->vkAllocationCallbacks);
#endif
        vkDestroyDescriptorSetLayout(backend->vkLogicalDevice, backend->vkDescriptorSetLayout, &backend->vkAllocationCallbacks);

        vulkan_cleanup_swap_chain               (backend);
        vulkan_destroy_fences                   (backend);
        vulkan_destroy_semaphores               (backend);
        vulkan_destroy_command_pool             (backend);
        vkDestroyDevice                         (backend->vkLogicalDevice, &backend->vkAllocationCallbacks);
        vkDestroySurfaceKHR                     (backend->vkInstance, backend->vkSurface, &backend->vkAllocationCallbacks);
        vulkan_destroy_debug_messenger          (backend);
        vkDestroyInstance                       (backend->vkInstance, &backend->vkAllocationCallbacks);
    }

    template<>
    void renderer_backend_handle_resize<RendererBackendVulkan>(RendererBackendVulkan* backend)
    {
        if (platform_window_is_minimized(backend->window))
        {
            return;
        }
        vulkan_recreate_swap_chain(backend);
    }

#endif
}
