
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <type_traits> // for std::remove_pointer_t

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

        // This stuff is constructed per renderer instance
        construct_memory_manager            (&backend->memoryManager, initData->bindings);
        construct_instance                  (&backend->instance, &backend->debugMessenger, initData, backend->memoryManager.cpu_allocationBindings, &backend->memoryManager.cpu_allocationCallbacks);
        construct_surface                   (&backend->surface, backend->window, backend->instance, &backend->memoryManager.cpu_allocationCallbacks);
        construct_gpu                       (&backend->gpu, backend->instance, backend->surface, backend->memoryManager.cpu_allocationBindings, &backend->memoryManager.cpu_allocationCallbacks);
        construct_swap_chain                (&backend->swapChain, backend->surface, &backend->gpu, backend->window, backend->memoryManager.cpu_allocationBindings, &backend->memoryManager.cpu_allocationCallbacks);
        construct_depth_stencil_attachment  (&backend->depthStencil, backend->swapChain.extent, &backend->gpu, &backend->memoryManager);        
        construct_command_pools             (backend, &backend->commandPools);
        construct_command_buffers           (backend, &backend->commandBuffers);
        create_sync_primitives              (backend);

        // This stuff is created by the user
        // Filling triangle buffer
        backend->_vertexBuffer = create_vertex_buffer(&backend->gpu, &backend->memoryManager, sizeof(triangle));
        backend->_indexBuffer = create_index_buffer(&backend->gpu, &backend->memoryManager, sizeof(triangleIndices));
        // Staging buffer occupies the whole memory chunk
        backend->_stagingBuffer = create_staging_buffer(&backend->gpu, &backend->memoryManager, VulkanMemoryManager::GPU_CHUNK_SIZE_BYTES);
        copy_cpu_memory_to_buffer(backend->gpu.logicalHandle, triangle, &backend->_stagingBuffer, sizeof(triangle));
        copy_buffer_to_buffer(&backend->gpu, &backend->commandBuffers, &backend->_stagingBuffer, &backend->_vertexBuffer, sizeof(triangle));
        copy_cpu_memory_to_buffer(backend->gpu.logicalHandle, triangleIndices, &backend->_stagingBuffer, sizeof(triangleIndices));
        copy_buffer_to_buffer(&backend->gpu, &backend->commandBuffers, &backend->_stagingBuffer, &backend->_indexBuffer, sizeof(triangleIndices));

        construct_color_attachment(&backend->_texture, { 5, 5 }, &backend->gpu, &backend->memoryManager);
        transition_image_layout(&backend->gpu, &backend->commandBuffers, backend->_texture.image, backend->_texture.format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        copy_cpu_memory_to_buffer(backend->gpu.logicalHandle, testEpicTexture, &backend->_stagingBuffer, sizeof(testEpicTexture));
        copy_buffer_to_image(&backend->gpu, &backend->commandBuffers, &backend->_stagingBuffer, backend->_texture.image, backend->_texture.extent);
        transition_image_layout(&backend->gpu, &backend->commandBuffers, backend->_texture.image, backend->_texture.format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        // Texture sampler
        {
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(backend->gpu.physicalHandle, &properties);
            VkSamplerCreateInfo samplerInfo
            {
                .sType                      = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                .pNext                      = nullptr,
                .flags                      = 0,
                .magFilter                  = VK_FILTER_NEAREST,
                .minFilter                  = VK_FILTER_NEAREST,
                .mipmapMode                 = VK_SAMPLER_MIPMAP_MODE_LINEAR,
                .addressModeU               = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                .addressModeV               = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                .addressModeW               = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                .mipLodBias                 = 0.0f,
                .anisotropyEnable           = VK_TRUE,
                .maxAnisotropy              = properties.limits.maxSamplerAnisotropy, // make anisotropy optional
                .compareEnable              = VK_FALSE,
                .compareOp                  = VK_COMPARE_OP_ALWAYS,
                .minLod                     = 0.0f,
                .maxLod                     = 0.0f,
                .borderColor                = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
                .unnormalizedCoordinates    = VK_FALSE,
            };
            al_vk_check(vkCreateSampler(backend->gpu.logicalHandle, &samplerInfo, &backend->memoryManager.cpu_allocationCallbacks, &backend->_textureSampler));

            // This stuff is costructed per pipeline instance
            construct_render_pass       (&backend->renderPass, &backend->swapChain, &backend->depthStencil, &backend->gpu, &backend->memoryManager);
            construct_framebuffers      (&backend->passFramebuffers, backend->renderPass, &backend->swapChain, &backend->memoryManager, &backend->gpu, &backend->depthStencil);
            create_test_descriptor_sets (backend, &backend->descriptorSets);
            construct_render_pipeline   (backend);
        }
    }

    template<>
    void renderer_backend_render<vulkan::RendererBackend>(vulkan::RendererBackend* backend)
    {
        using namespace vulkan;
        backend->currentRenderFrame = utils::advance_render_frame(backend->currentRenderFrame);
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
                    .renderPass         = backend->renderPass,
                    .framebuffer        = backend->passFramebuffers[swapChainImageIndex],
                    .renderArea         = { .offset = { 0, 0 }, .extent = backend->swapChain.extent },
                    .clearValueCount    = array_size(clearValues),
                    .pClearValues       = clearValues,
                };
                vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, backend->pipeline);
                static f32 tint = 0.0f;
                tint += 0.0001f;
                if (tint > 1.0f)
                {
                    tint = 0.0f;
                }
                f32 color = tint;
                f32_3 testPushConstant = { color, color, color };
                vkCmdPushConstants(commandBuffer, backend->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(f32_3), &testPushConstant);
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, backend->pipelineLayout, 0, 1, &backend->descriptorSets.sets[swapChainImageIndex], 0, nullptr);
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
                .waitSemaphoreCount     = array_size(waitSemaphores),
                .pWaitSemaphores        = waitSemaphores,
                .pWaitDstStageMask      = waitStages,
                .commandBufferCount     = 1,
                .pCommandBuffers        = &commandBuffer,
                .signalSemaphoreCount   = array_size(signalSemaphores),
                .pSignalSemaphores      = signalSemaphores,
            };
            vkResetFences(backend->gpu.logicalHandle, 1, &backend->syncPrimitives.inFlightFences[backend->currentRenderFrame]);
            vkQueueSubmit(backend->gpu.graphicsQueue.handle, 1, &submitInfo, backend->syncPrimitives.inFlightFences[backend->currentRenderFrame]);
            VkSwapchainKHR swapChains[] =
            {
                backend->swapChain.handle
            };
            VkPresentInfoKHR presentInfo
            {
                .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                .pNext              = nullptr,
                .waitSemaphoreCount = array_size(signalSemaphores),
                .pWaitSemaphores    = signalSemaphores,
                .swapchainCount     = array_size(swapChains),
                .pSwapchains        = swapChains,
                .pImageIndices      = &swapChainImageIndex,
                .pResults           = nullptr,
            };
            al_vk_check(vkQueuePresentKHR(backend->gpu.presentQueue.handle, &presentInfo));
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

        vkDestroySampler(backend->gpu.logicalHandle, backend->_textureSampler, &backend->memoryManager.cpu_allocationCallbacks);
        destroy_image_attachment(&backend->_texture, &backend->gpu, &backend->memoryManager);

        destroy_buffer(&backend->gpu, &backend->memoryManager, &backend->_indexBuffer);
        destroy_buffer(&backend->gpu, &backend->memoryManager, &backend->_vertexBuffer);
        destroy_buffer(&backend->gpu, &backend->memoryManager, &backend->_stagingBuffer);

        destroy_render_pipeline     (backend);
        destroy_test_descriptor_sets(backend, &backend->descriptorSets);
        destroy_framebuffers        (backend->passFramebuffers, &backend->gpu, &backend->memoryManager);
        destroy_render_pass         (backend->renderPass, &backend->gpu, &backend->memoryManager);

        destroy_sync_primitives (backend);
        destroy_command_buffers (backend, &backend->commandBuffers);
        destroy_command_pools   (backend, &backend->commandPools);
        destroy_image_attachment(&backend->depthStencil, &backend->gpu, &backend->memoryManager);
        destroy_swap_chain      (&backend->swapChain, &backend->gpu, &backend->memoryManager.cpu_allocationCallbacks);
        destroy_memory_manager  (&backend->memoryManager, backend->gpu.logicalHandle);
        destroy_gpu             (&backend->gpu, &backend->memoryManager.cpu_allocationCallbacks);
        destroy_surface         (backend->surface, backend->instance, &backend->memoryManager.cpu_allocationCallbacks);
        destroy_instance        (backend->instance, backend->debugMessenger, &backend->memoryManager.cpu_allocationCallbacks);
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
            destroy_render_pipeline(backend);
            destroy_framebuffers(backend->passFramebuffers, &backend->gpu, &backend->memoryManager);
            destroy_render_pass       (backend->renderPass, &backend->gpu, &backend->memoryManager);
            destroy_image_attachment(&backend->depthStencil, &backend->gpu, &backend->memoryManager);
            destroy_swap_chain      (&backend->swapChain, &backend->gpu, &backend->memoryManager.cpu_allocationCallbacks);
        }
        {
            construct_swap_chain    (&backend->swapChain, backend->surface, &backend->gpu, backend->window, backend->memoryManager.cpu_allocationBindings, &backend->memoryManager.cpu_allocationCallbacks);
            construct_depth_stencil_attachment (&backend->depthStencil, backend->swapChain.extent, &backend->gpu, &backend->memoryManager);
            construct_render_pass            (&backend->renderPass, &backend->swapChain, &backend->depthStencil, &backend->gpu, &backend->memoryManager);
            construct_framebuffers      (&backend->passFramebuffers, backend->renderPass, &backend->swapChain, &backend->memoryManager, &backend->gpu, &backend->depthStencil);
            construct_render_pipeline (backend);
        }
    }
}

namespace al::vulkan
{
    // =================================================================================================================
    // CREATION
    // =================================================================================================================

    void construct_instance(VkInstance* instance, VkDebugUtilsMessengerEXT* debugMessenger, RendererBackendInitData* initData, AllocatorBindings bindings, VkAllocationCallbacks* callbacks)
    {
#ifdef AL_DEBUG
        {
            ArrayView<VkLayerProperties> availableValidationLayers = utils::get_available_validation_layers(&bindings);
            defer(av_destruct(availableValidationLayers));
            for (uSize requiredIt = 0; requiredIt < array_size(utils::VALIDATION_LAYERS); requiredIt++)
            {
                const char* requiredLayerName = utils::VALIDATION_LAYERS[requiredIt];
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
            ArrayView<VkExtensionProperties> availableInstanceExtensions = utils::get_available_instance_extensions(&bindings);
            defer(av_destruct(availableInstanceExtensions));
            for (uSize requiredIt = 0; requiredIt < array_size(utils::INSTANCE_EXTENSIONS); requiredIt++)
            {
                const char* requiredInstanceExtensionName = utils::INSTANCE_EXTENSIONS[requiredIt];
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
        instanceCreateInfo.enabledExtensionCount    = array_size(utils::INSTANCE_EXTENSIONS);
        instanceCreateInfo.ppEnabledExtensionNames  = utils::INSTANCE_EXTENSIONS;
#ifdef AL_DEBUG
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = utils::get_debug_messenger_create_info(vulkan_debug_callback);
        instanceCreateInfo.pNext                = &debugCreateInfo;
        instanceCreateInfo.enabledLayerCount    = array_size(utils::VALIDATION_LAYERS);
        instanceCreateInfo.ppEnabledLayerNames  = utils::VALIDATION_LAYERS;
#endif
        al_vk_check(vkCreateInstance(&instanceCreateInfo, callbacks, instance));
#ifdef AL_DEBUG
        *debugMessenger = utils::create_debug_messenger(&debugCreateInfo, *instance, callbacks);
#endif
    }

    void construct_surface(VkSurfaceKHR* surface, PlatformWindow* window, VkInstance instance, VkAllocationCallbacks* allocationCallbacks)
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
            al_vk_check(vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, allocationCallbacks, surface));
        }
#else
#   error Unsupported platform
#endif
    }

    void construct_render_pass(VkRenderPass* pass, SwapChain* swapChain, ImageAttachment* depthStencil, GPU* gpu, VulkanMemoryManager* memoryManager)
    {
        // Descriptors for the attachments used by this renderpass
        VkAttachmentDescription attachments[2] = { };

        // Color attachment
        attachments[0].format           = swapChain->format;                    // Use the color format selected by the swapchain
        attachments[0].samples          = VK_SAMPLE_COUNT_1_BIT;                // We don't use multi sampling in this example
        attachments[0].loadOp           = VK_ATTACHMENT_LOAD_OP_CLEAR;          // Clear this attachment at the start of the render pass
        attachments[0].storeOp          = VK_ATTACHMENT_STORE_OP_STORE;         // Keep its contents after the render pass is finished (for displaying it)
        attachments[0].stencilLoadOp    = VK_ATTACHMENT_LOAD_OP_DONT_CARE;      // We don't use stencil, so don't care for load
        attachments[0].stencilStoreOp   = VK_ATTACHMENT_STORE_OP_DONT_CARE;     // Same for store
        attachments[0].initialLayout    = VK_IMAGE_LAYOUT_UNDEFINED;            // Layout at render pass start. Initial doesn't matter, so we use undefined
        attachments[0].finalLayout      = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;      // Layout to which the attachment is transitioned when the render pass is finished
                                                                                // As we want to present the color buffer to the swapchain, we transition to PRESENT_KHR
        // Depth attachment
        attachments[1].format           = depthStencil->format;
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

        al_vk_check(vkCreateRenderPass(gpu->logicalHandle, &renderPassInfo, &memoryManager->cpu_allocationCallbacks, pass));
    }

    void construct_framebuffers(ArrayView<VkFramebuffer>* framebuffers, VkRenderPass pass, SwapChain* swapChain, VulkanMemoryManager* memoryManager, GPU* gpu, ImageAttachment* depthStencil)
    {
        av_construct(framebuffers, &memoryManager->cpu_allocationBindings, swapChain->images.count);
        for (uSize it = 0; it < swapChain->images.count; it++)
        {
            VkImageView attachments[] =
            {
                swapChain->imageViews[it],
                depthStencil->view
            };
            VkFramebufferCreateInfo framebufferInfo
            {
                .sType              = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .pNext              = nullptr,
                .flags              = 0,
                .renderPass         = pass,
                .attachmentCount    = array_size(attachments),
                .pAttachments       = attachments,
                .width              = swapChain->extent.width,
                .height             = swapChain->extent.height,
                .layers             = 1,
            };
            al_vk_check(vkCreateFramebuffer(gpu->logicalHandle, &framebufferInfo, &memoryManager->cpu_allocationCallbacks, &(*framebuffers)[it]));
        }
    }

    void construct_render_pipeline(RendererBackend* backend)
    {
        VulkanMemoryManager*        memoryManager   = &backend->memoryManager;
        GPU*                        gpu             = &backend->gpu;
        SwapChain*                  swapChain       = &backend->swapChain;
        VkRenderPass                pass            =  backend->renderPass;
        VkPipelineLayout*           layout          = &backend->pipelineLayout;
        VkPipeline*                 pipeline        = &backend->pipeline;
        HardcodedDescriptorsSets*   descriptorSets  = &backend->descriptorSets;

        PlatformFile vertShader = platform_file_load(memoryManager->cpu_allocationBindings, RendererBackend::VERTEX_SHADER_PATH, PlatformFileLoadMode::READ);
        PlatformFile fragShader = platform_file_load(memoryManager->cpu_allocationBindings, RendererBackend::FRAGMENT_SHADER_PATH, PlatformFileLoadMode::READ);
        defer(platform_file_unload(memoryManager->cpu_allocationBindings, vertShader));
        defer(platform_file_unload(memoryManager->cpu_allocationBindings, fragShader));
        VkShaderModule vertShaderModule = utils::create_shader_module(gpu->logicalHandle, { static_cast<u32*>(vertShader.memory), vertShader.sizeBytes }, &memoryManager->cpu_allocationCallbacks);
        VkShaderModule fragShaderModule = utils::create_shader_module(gpu->logicalHandle, { static_cast<u32*>(fragShader.memory), fragShader.sizeBytes }, &memoryManager->cpu_allocationCallbacks);
        defer(utils::destroy_shader_module(gpu->logicalHandle, vertShaderModule, &memoryManager->cpu_allocationCallbacks));
        defer(utils::destroy_shader_module(gpu->logicalHandle, fragShaderModule, &memoryManager->cpu_allocationCallbacks));
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
            .width      = static_cast<float>(swapChain->extent.width),
            .height     = static_cast<float>(swapChain->extent.height),
            .minDepth   = 0.0f,
            .maxDepth   = 1.0f,
        };
        VkRect2D scissor
        {
            .offset = { 0, 0 },
            .extent = swapChain->extent,
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
        // Push constants can be retrieved from reflection data
        VkPushConstantRange testPushContantRanges[] =
        {
            {
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                .offset     = 0,
                .size       = sizeof(f32_3),
            }
        };
        VkPipelineLayoutCreateInfo pipelineLayoutInfo
        {
            .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext                  = nullptr,
            .flags                  = 0,
            .setLayoutCount         = 1,
            .pSetLayouts            = &descriptorSets->layout,
            .pushConstantRangeCount = array_size(testPushContantRanges),
            .pPushConstantRanges    = testPushContantRanges,
        };
        al_vk_check(vkCreatePipelineLayout(gpu->logicalHandle, &pipelineLayoutInfo, &memoryManager->cpu_allocationCallbacks, layout));
        VkGraphicsPipelineCreateInfo pipelineInfo
        {
            .sType                  = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext                  = nullptr,
            .flags                  = 0,
            .stageCount             = array_size(shaderStages),
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
            .layout                 = *layout,
            .renderPass             = pass,
            .subpass                = 0,
            .basePipelineHandle     = VK_NULL_HANDLE,
            .basePipelineIndex      = -1,
        };
        al_vk_check(vkCreateGraphicsPipelines(gpu->logicalHandle, VK_NULL_HANDLE, 1, &pipelineInfo, &memoryManager->cpu_allocationCallbacks, pipeline));
    }

    void create_sync_primitives(RendererBackend* backend)
    {
        // Semaphores
        av_construct(&backend->syncPrimitives.imageAvailableSemaphores, &backend->memoryManager.cpu_allocationBindings, utils::MAX_IMAGES_IN_FLIGHT);
        av_construct(&backend->syncPrimitives.renderFinishedSemaphores, &backend->memoryManager.cpu_allocationBindings, utils::MAX_IMAGES_IN_FLIGHT);
        for (uSize it = 0; it < utils::MAX_IMAGES_IN_FLIGHT; it++)
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
        av_construct(&backend->syncPrimitives.inFlightFences, &backend->memoryManager.cpu_allocationBindings, utils::MAX_IMAGES_IN_FLIGHT);
        for (uSize it = 0; it < utils::MAX_IMAGES_IN_FLIGHT; it++)
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

    void destroy_instance(VkInstance instance, VkDebugUtilsMessengerEXT messenger, VkAllocationCallbacks* callbacks)
    {
#ifdef AL_DEBUG
        utils::destroy_debug_messenger(instance, messenger, callbacks);
#endif
        vkDestroyInstance(instance, callbacks);
    }

    void destroy_surface(VkSurfaceKHR surface, VkInstance instance, VkAllocationCallbacks* callbacks)
    {
        vkDestroySurfaceKHR(instance, surface, callbacks);
    }

    void destroy_framebuffers(ArrayView<VkFramebuffer> framebuffers, GPU* gpu, VulkanMemoryManager* memoryManager)
    {
        for (uSize it = 0; it < framebuffers.count; it++)
        {
            vkDestroyFramebuffer(gpu->logicalHandle, framebuffers[it], &memoryManager->cpu_allocationCallbacks);
        }
        av_destruct(framebuffers);
    }

    void destroy_render_pass(VkRenderPass pass, GPU* gpu, VulkanMemoryManager* memoryManager)
    {
        vkDestroyRenderPass(gpu->logicalHandle, pass, &memoryManager->cpu_allocationCallbacks);
    }

    void destroy_render_pipeline(RendererBackend* backend)
    {
        VulkanMemoryManager*    memoryManager   = &backend->memoryManager;
        GPU*                    gpu             = &backend->gpu;
        VkPipelineLayout        layout          =  backend->pipelineLayout;
        VkPipeline              pipeline        =  backend->pipeline;

        vkDestroyPipeline(gpu->logicalHandle, pipeline, &memoryManager->cpu_allocationCallbacks);
        vkDestroyPipelineLayout(gpu->logicalHandle, layout, &memoryManager->cpu_allocationCallbacks);
    }

    void destroy_sync_primitives(RendererBackend* backend)
    {
        // Semaphores
        for (uSize it = 0; it < utils::MAX_IMAGES_IN_FLIGHT; it++)
        {
            vkDestroySemaphore(backend->gpu.logicalHandle, backend->syncPrimitives.imageAvailableSemaphores[it], &backend->memoryManager.cpu_allocationCallbacks);
            vkDestroySemaphore(backend->gpu.logicalHandle, backend->syncPrimitives.renderFinishedSemaphores[it], &backend->memoryManager.cpu_allocationCallbacks);
        }
        av_destruct(backend->syncPrimitives.imageAvailableSemaphores);
        av_destruct(backend->syncPrimitives.renderFinishedSemaphores);
        // Fences
        for (uSize it = 0; it < utils::MAX_IMAGES_IN_FLIGHT; it++)
        {
            vkDestroyFence(backend->gpu.logicalHandle, backend->syncPrimitives.inFlightFences[it], &backend->memoryManager.cpu_allocationCallbacks);
        }
        av_destruct(backend->syncPrimitives.inFlightFences);
        av_destruct(backend->syncPrimitives.imageInFlightFencesRef);
    }






















    

    void construct_memory_manager(VulkanMemoryManager* memoryManager, AllocatorBindings bindings)
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

    void destroy_memory_manager(VulkanMemoryManager* memoryManager, VkDevice device)
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

    uSize memory_chunk_find_aligned_free_space(VulkanMemoryManager::GpuMemoryChunk* chunk, uSize requiredNumberOfBlocks, uSize alignment)
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
                        uSize currentBlockOffsetBytes = (ledgerIt * 8 + byteIt) * VulkanMemoryManager::GPU_MEMORY_BLOCK_SIZE_BYTES;
                        bool isAlignedCorrectly = (currentBlockOffsetBytes % alignment) == 0;
                        if (isAlignedCorrectly)
                        {
                            startBlock = currentBlock;
                            freeCount += 1;
                        }
                    }
                    else
                    {
                        freeCount += 1;
                    }
                }
                currentBlock += 1;
                if (freeCount == requiredNumberOfBlocks)
                {
                    goto block_found;
                }
            }
        }
        startBlock = VulkanMemoryManager::GPU_LEDGER_SIZE_BYTES;
        block_found:
        return startBlock;
    }

    GpuMemory gpu_allocate(VulkanMemoryManager* memoryManager, VkDevice device, VulkanMemoryManager::GpuAllocationRequest request)
    {
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
        uSize requiredNumberOfBlocks = 1 + ((request.sizeBytes - 1) / VulkanMemoryManager::GPU_MEMORY_BLOCK_SIZE_BYTES);
        // 1. Try to find memory in available chunks
        for (uSize it = 0; it < VulkanMemoryManager::GPU_MAX_CHUNKS; it++)
        {
            VulkanMemoryManager::GpuMemoryChunk* chunk = &memoryManager->gpu_chunks[it];
            if (!chunk->memory)
            {
                continue;
            }
            if (chunk->memoryTypeIndex != request.memoryTypeIndex)
            {
                continue;
            }
            if ((VulkanMemoryManager::GPU_CHUNK_SIZE_BYTES - chunk->usedMemoryBytes) < request.sizeBytes)
            {
                continue;
            }
            uSize inChunkOffset = memory_chunk_find_aligned_free_space(chunk, requiredNumberOfBlocks, request.alignment);
            if (inChunkOffset == VulkanMemoryManager::GPU_LEDGER_SIZE_BYTES)
            {
                continue;
            }
            setInUse(chunk, requiredNumberOfBlocks, inChunkOffset);
            return
            {
                .memory = chunk->memory,
                .offsetBytes = inChunkOffset * VulkanMemoryManager::GPU_MEMORY_BLOCK_SIZE_BYTES,
                .sizeBytes = requiredNumberOfBlocks * VulkanMemoryManager::GPU_MEMORY_BLOCK_SIZE_BYTES,
            };
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
            {
                VkMemoryAllocateInfo memoryAllocateInfo = { };
                memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                memoryAllocateInfo.allocationSize = VulkanMemoryManager::GPU_CHUNK_SIZE_BYTES;
                memoryAllocateInfo.memoryTypeIndex = request.memoryTypeIndex;
                al_vk_check(vkAllocateMemory(device, &memoryAllocateInfo, &memoryManager->cpu_allocationCallbacks, &chunk->memory));
                chunk->memoryTypeIndex = request.memoryTypeIndex;
                std::memset(chunk->ledger, 0, VulkanMemoryManager::GPU_LEDGER_SIZE_BYTES);
            }
            // Allocating memory in new chunk
            uSize inChunkOffset = memory_chunk_find_aligned_free_space(chunk, requiredNumberOfBlocks, request.alignment);
            al_vk_assert(inChunkOffset != VulkanMemoryManager::GPU_LEDGER_SIZE_BYTES);
            setInUse(chunk, requiredNumberOfBlocks, inChunkOffset);
            return
            {
                .memory = chunk->memory,
                .offsetBytes = inChunkOffset * VulkanMemoryManager::GPU_MEMORY_BLOCK_SIZE_BYTES,
                .sizeBytes = requiredNumberOfBlocks * VulkanMemoryManager::GPU_MEMORY_BLOCK_SIZE_BYTES,
            };
        }
        al_vk_assert(!"Out of memory");
        return { };
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

    MemoryBuffer create_buffer(GPU* gpu, VulkanMemoryManager* memoryManager, VkBufferCreateInfo* createInfo, VkMemoryPropertyFlags memoryProperty)
    {
        MemoryBuffer buffer{ };
        al_vk_check(vkCreateBuffer(gpu->logicalHandle, createInfo, &memoryManager->cpu_allocationCallbacks, &buffer.handle));
        VkMemoryRequirements memoryRequirements;
        vkGetBufferMemoryRequirements(gpu->logicalHandle, buffer.handle, &memoryRequirements);
        u32 memoryTypeIndex;
        utils::get_memory_type_index(&gpu->memoryProperties, memoryRequirements.memoryTypeBits, memoryProperty, &memoryTypeIndex);
        buffer.gpuMemory = gpu_allocate(memoryManager, gpu->logicalHandle, { .sizeBytes = memoryRequirements.size, .alignment = memoryRequirements.alignment, .memoryTypeIndex = memoryTypeIndex });
        al_vk_check(vkBindBufferMemory(gpu->logicalHandle, buffer.handle, buffer.gpuMemory.memory, buffer.gpuMemory.offsetBytes));
        return buffer;
    }

    MemoryBuffer create_vertex_buffer(GPU* gpu, VulkanMemoryManager* memoryManager, uSize sizeSytes)
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
            gpu->transferQueue.deviceFamilyIndex,
            gpu->graphicsQueue.deviceFamilyIndex
        };
        if (gpu->transferQueue.deviceFamilyIndex != gpu->graphicsQueue.deviceFamilyIndex)
        {
            bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
            bufferInfo.queueFamilyIndexCount = 2;
            bufferInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        return create_buffer(gpu, memoryManager, &bufferInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }

    MemoryBuffer create_index_buffer(GPU* gpu, VulkanMemoryManager* memoryManager, uSize sizeSytes)
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
            gpu->transferQueue.deviceFamilyIndex,
            gpu->graphicsQueue.deviceFamilyIndex
        };
        if (gpu->transferQueue.deviceFamilyIndex != gpu->graphicsQueue.deviceFamilyIndex)
        {
            bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
            bufferInfo.queueFamilyIndexCount = 2;
            bufferInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        return create_buffer(gpu, memoryManager, &bufferInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }

    MemoryBuffer create_staging_buffer(GPU* gpu, VulkanMemoryManager* memoryManager, uSize sizeSytes)
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
        return create_buffer(gpu, memoryManager, &bufferInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }

    MemoryBuffer create_uniform_buffer(GPU* gpu, VulkanMemoryManager* memoryManager, uSize sizeSytes)
    {
        VkBufferCreateInfo bufferInfo
        {
            .sType                  = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext                  = nullptr,
            .flags                  = 0,
            .size                   = sizeSytes,
            .usage                  = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            .sharingMode            = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount  = 0,        // ignored if sharingMode is not VK_SHARING_MODE_CONCURRENT
            .pQueueFamilyIndices    = nullptr,  // ignored if sharingMode is not VK_SHARING_MODE_CONCURRENT
        };
        return create_buffer(gpu, memoryManager, &bufferInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }

    void destroy_buffer(GPU* gpu, VulkanMemoryManager* memoryManager, MemoryBuffer* buffer)
    {
        gpu_deallocate(memoryManager, gpu->logicalHandle, buffer->gpuMemory);
        vkDestroyBuffer(gpu->logicalHandle, buffer->handle, &memoryManager->cpu_allocationCallbacks);
        std::memset(&buffer, 0, sizeof(MemoryBuffer));
    }

    void copy_cpu_memory_to_buffer(VkDevice device, void* data, MemoryBuffer* buffer, uSize dataSizeBytes)
    {
        void* mappedMemory;
        vkMapMemory(device, buffer->gpuMemory.memory, buffer->gpuMemory.offsetBytes, dataSizeBytes, 0, &mappedMemory);
        std::memcpy(mappedMemory, data, dataSizeBytes);
        vkUnmapMemory(device, buffer->gpuMemory.memory);
    }

    void copy_buffer_to_buffer(GPU* gpu, CommandBuffers* buffers, MemoryBuffer* src, MemoryBuffer* dst, uSize sizeBytes, uSize srcOffsetBytes, uSize dstOffsetBytes)
    {
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
        vkQueueSubmit(gpu->transferQueue.handle, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(gpu->transferQueue.handle);
    }

    void copy_buffer_to_image(GPU* gpu, CommandBuffers* buffers, MemoryBuffer* src, VkImage dst, VkExtent3D extent)
    {
        VkCommandBufferBeginInfo beginInfo
        {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext              = nullptr,
            .flags              = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            .pInheritanceInfo   = nullptr,
        };
        vkBeginCommandBuffer(buffers->transferBuffer, &beginInfo);
        VkBufferImageCopy imageCopy
        {
            .bufferOffset       = 0,
            .bufferRowLength    = 0,
            .bufferImageHeight  = 0,
            .imageSubresource   =
            {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,    // @TODO : unhardcode this
                .mipLevel       = 0,
                .baseArrayLayer = 0,
                .layerCount     = 1,
            },
            .imageOffset        = 0,
            .imageExtent        = extent,
        };
        vkCmdCopyBufferToImage(buffers->transferBuffer, src->handle, dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopy);
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
        vkQueueSubmit(gpu->transferQueue.handle, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(gpu->transferQueue.handle);
    }

    bool is_command_queues_complete(ArrayView<GPU::CommandQueue> queues)
    {
        for (uSize it = 0; it < queues.count; it++)
        {
            if (!queues[it].isFamilyPresent)
            {
                return false;
            }
        }
        return true;
    };

    void try_pick_graphics_queue(GPU::CommandQueue* queue, ArrayView<VkQueueFamilyProperties> familyProperties)
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

    void try_pick_present_queue(GPU::CommandQueue* queue, ArrayView<VkQueueFamilyProperties> familyProperties, VkPhysicalDevice device, VkSurfaceKHR surface)
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

    void try_pick_transfer_queue(GPU::CommandQueue* queue, ArrayView<VkQueueFamilyProperties> familyProperties, u32 graphicsFamilyIndex)
    {
        //
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
        //
        // If there is no separate queue family, use graphics family (if possible (propbably this is possible almost always))
        if (familyProperties[graphicsFamilyIndex].queueFlags & VK_QUEUE_TRANSFER_BIT)
        {
            queue->deviceFamilyIndex = graphicsFamilyIndex;
            queue->isFamilyPresent = true;
        }
    }

    void fill_required_physical_deivce_features(VkPhysicalDeviceFeatures* features)
    {
        // @TODO : fill features struct
        features->samplerAnisotropy = true;
    }

    VkPhysicalDevice pick_physical_device(VkInstance instance, VkSurfaceKHR surface, AllocatorBindings bindings)
    {
        auto isDeviceSuitable = [](VkPhysicalDevice device, VkSurfaceKHR surface, AllocatorBindings bindings) -> bool
        {
            //
            // Check if all required queues are supported
            ArrayView<VkQueueFamilyProperties> familyProperties = utils::get_physical_device_queue_family_properties(device, bindings);
            defer(av_destruct(familyProperties));
            GPU::CommandQueue physicalDeviceCommandQueues[array_size(GPU::commandQueues)] = {};
            try_pick_graphics_queue(&physicalDeviceCommandQueues[0], familyProperties);
            try_pick_present_queue(&physicalDeviceCommandQueues[1], familyProperties, device, surface);
            if (physicalDeviceCommandQueues[0].isFamilyPresent)
            {
                try_pick_transfer_queue(&physicalDeviceCommandQueues[2], familyProperties, physicalDeviceCommandQueues[0].deviceFamilyIndex);
            }
            ArrayView<const char* const> deviceExtensions
            {
                .bindings   = { },
                .memory     = utils::DEVICE_EXTENSIONS,
                .count      = array_size(utils::DEVICE_EXTENSIONS),
            };
            //
            // Check if required extensions are supported
            bool isRequiredExtensionsSupported = utils::does_physical_device_supports_required_extensions(device, deviceExtensions, bindings);
            //
            // Check if swapchain is supported
            bool isSwapChainSuppoted = false;
            if (isRequiredExtensionsSupported)
            {
                SwapChainSupportDetails supportDetails = create_swap_chain_support_details(surface, device, bindings);
                isSwapChainSuppoted = supportDetails.formats.count && supportDetails.presentModes.count;
                destroy_swap_chain_support_details(&supportDetails);
            }
            //
            // Check if all device features are supported
            VkPhysicalDeviceFeatures requiredFeatures{ };
            fill_required_physical_deivce_features(&requiredFeatures);
            bool isRequiredFeaturesSupported = utils::does_physical_device_supports_required_features(device, &requiredFeatures);
            return
            {
                is_command_queues_complete({ .bindings = { }, .memory = physicalDeviceCommandQueues, .count = array_size(GPU::commandQueues)})
                && isRequiredExtensionsSupported && isSwapChainSuppoted && isRequiredFeaturesSupported
            };
        };
        ArrayView<VkPhysicalDevice> available = utils::get_available_physical_devices(instance, bindings);
        defer(av_destruct(available));
        for (uSize it = 0; it < available.count; it++)
        {
            if (isDeviceSuitable(available[it], surface, bindings))
            {
                // @TODO :  log device name and shit
                // VkPhysicalDeviceProperties deviceProperties;
                // vkGetPhysicalDeviceProperties(chosenPhysicalDevice, &deviceProperties);
                return available[it];
            }
        }
        return
        {
            VK_NULL_HANDLE
        };
    }

    void construct_gpu(GPU* gpu, VkInstance instance, VkSurfaceKHR surface, AllocatorBindings bindings, VkAllocationCallbacks* callbacks)
    {
        auto getQueueCreateInfos = [](GPU::CommandQueue* queues, AllocatorBindings bindings) -> ArrayView<VkDeviceQueueCreateInfo>
        {
            // @NOTE :  this is possible that queue family might support more than one of the required features,
            //          so we have to remove duplicates from queueFamiliesInfo and create VkDeviceQueueCreateInfos
            //          only for unique indexes
            static const f32 QUEUE_DEFAULT_PRIORITY = 1.0f;
            u32 indicesArray[array_size(GPU::commandQueues)];
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
            for (uSize it = 0; it < array_size(GPU::commandQueues); it++)
            {
                updateUniqueIndicesArray(&uniqueQueueIndices, queues[it].deviceFamilyIndex);
            }
            ArrayView<VkDeviceQueueCreateInfo> result;
            av_construct(&result, &bindings, uniqueQueueIndices.count);
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
        //
        // Get physical device
        gpu->physicalHandle = pick_physical_device(instance, surface, bindings);
        al_vk_assert(gpu->physicalHandle != VK_NULL_HANDLE);
        //
        // Get command queue infos
        ArrayView<VkQueueFamilyProperties> familyProperties = utils::get_physical_device_queue_family_properties(gpu->physicalHandle, bindings);
        defer(av_destruct(familyProperties));
        try_pick_graphics_queue(&gpu->graphicsQueue, familyProperties);
        try_pick_present_queue(&gpu->presentQueue, familyProperties, gpu->physicalHandle, surface);
        try_pick_transfer_queue(&gpu->transferQueue, familyProperties, gpu->graphicsQueue.deviceFamilyIndex);
        //
        // Create logical device
        ArrayView<VkDeviceQueueCreateInfo> queueCreateInfos = getQueueCreateInfos(gpu->commandQueues, bindings);
        defer(av_destruct(queueCreateInfos));
        VkPhysicalDeviceFeatures deviceFeatures{ };
        fill_required_physical_deivce_features(&deviceFeatures);
        VkDeviceCreateInfo logicalDeviceCreateInfo
        {
            .sType                      = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext                      = nullptr,
            .flags                      = 0,
            .queueCreateInfoCount       = static_cast<u32>(queueCreateInfos.count),
            .pQueueCreateInfos          = queueCreateInfos.memory,
            // @NOTE : Validation layers info is not used by recent vulkan implemetations, but still can be set for compatibility reasons
            .enabledLayerCount          = array_size(utils::VALIDATION_LAYERS),
            .ppEnabledLayerNames        = utils::VALIDATION_LAYERS,
            .enabledExtensionCount      = array_size(utils::DEVICE_EXTENSIONS),
            .ppEnabledExtensionNames    = utils::DEVICE_EXTENSIONS,
            .pEnabledFeatures           = &deviceFeatures
        };
        al_vk_check(vkCreateDevice(gpu->physicalHandle, &logicalDeviceCreateInfo, callbacks, &gpu->logicalHandle));
        //
        // Create queues and get other stuff
        for (uSize it = 0; it < array_size(GPU::commandQueues); it++)
        {
            vkGetDeviceQueue(gpu->logicalHandle, gpu->commandQueues[it].deviceFamilyIndex, 0, &gpu->commandQueues[it].handle);
        }
        utils::pick_depth_format(gpu->physicalHandle, &gpu->depthStencilFormat);
        vkGetPhysicalDeviceMemoryProperties(gpu->physicalHandle, &gpu->memoryProperties);
    }

    void destroy_gpu(GPU* gpu, VkAllocationCallbacks* callbacks)
    {
        vkDestroyDevice(gpu->logicalHandle, callbacks);
    }

    SwapChainSupportDetails create_swap_chain_support_details(VkSurfaceKHR surface, VkPhysicalDevice device, AllocatorBindings bindings)
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

    void destroy_swap_chain_support_details(SwapChainSupportDetails* details)
    {
        av_destruct(details->formats);
        av_destruct(details->presentModes);
    }

    VkSurfaceFormatKHR choose_swap_chain_surface_format(ArrayView<VkSurfaceFormatKHR> available)
    {
        for (uSize it = 0; it < available.count; it++)
        {
            if (available[it].format == VK_FORMAT_B8G8R8A8_SRGB && available[it].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return available[it];
            }
        }
        return available[0];
    }

    VkPresentModeKHR choose_swap_chain_surface_present_mode(ArrayView<VkPresentModeKHR> available)
    {
        for (uSize it = 0; it < available.count; it++)
        {
            if (available[it] == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                return available[it];
            }
        }
        return VK_PRESENT_MODE_FIFO_KHR; // Guarateed to be available
    }

    VkExtent2D choose_swap_chain_extent(u32 windowWidth, u32 windowHeight, VkSurfaceCapabilitiesKHR* capabilities)
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
                clamp(capabilities->minImageExtent.width , capabilities->maxImageExtent.width , windowWidth),
                clamp(capabilities->minImageExtent.height, capabilities->maxImageExtent.height, windowHeight)
            };
        }
    }

    void fill_swap_chain_sharing_mode(VkSharingMode* resultMode, u32* resultCount, ArrayView<u32> resultFamilyIndices, GPU* gpu)
    {
        al_vk_assert(resultFamilyIndices.count >= 2);
        if (gpu->graphicsQueue.deviceFamilyIndex == gpu->presentQueue.deviceFamilyIndex)
        {
            *resultMode = VK_SHARING_MODE_EXCLUSIVE;
            *resultCount = 0;
        }
        else
        {
            *resultMode = VK_SHARING_MODE_CONCURRENT;
            *resultCount = 2;
            resultFamilyIndices[0] = gpu->graphicsQueue.deviceFamilyIndex;
            resultFamilyIndices[1] = gpu->presentQueue.deviceFamilyIndex;
        }
    }

    void construct_swap_chain(SwapChain* swapChain, VkSurfaceKHR surface, GPU* gpu, PlatformWindow* window, AllocatorBindings bindings, VkAllocationCallbacks* callbacks)
    {
        {
            SwapChainSupportDetails supportDetails = create_swap_chain_support_details(surface, gpu->physicalHandle, bindings);
            defer(destroy_swap_chain_support_details(&supportDetails));
            VkSurfaceFormatKHR  surfaceFormat   = choose_swap_chain_surface_format(supportDetails.formats);
            VkPresentModeKHR    presentMode     = choose_swap_chain_surface_present_mode(supportDetails.presentModes);
            VkExtent2D          extent          = choose_swap_chain_extent(platform_window_get_current_width(window), platform_window_get_current_width(window), &supportDetails.capabilities);
            // @NOTE :  supportDetails.capabilities.maxImageCount == 0 means unlimited amount of images in swapchain
            u32 imageCount = supportDetails.capabilities.minImageCount + 1;
            if (supportDetails.capabilities.maxImageCount != 0 && imageCount > supportDetails.capabilities.maxImageCount)
            {
                imageCount = supportDetails.capabilities.maxImageCount;
            }
            u32 queueFamiliesIndices[array_size(GPU::commandQueues)];
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
                .imageArrayLayers       = 1, // "This is always 1 unless you are developing a stereoscopic 3D application"
                .imageUsage             = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                .imageSharingMode       = VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount  = 0,
                .pQueueFamilyIndices    = queueFamiliesIndices,
                .preTransform           = supportDetails.capabilities.currentTransform, // Allows to apply transform to images (rotate etc)
                .compositeAlpha         = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                .presentMode            = presentMode,
                .clipped                = VK_TRUE,
                .oldSwapchain           = VK_NULL_HANDLE,
            };
            fill_swap_chain_sharing_mode(&createInfo.imageSharingMode, &createInfo.queueFamilyIndexCount, { .bindings = {}, .memory = queueFamiliesIndices, .count = array_size(queueFamiliesIndices) }, gpu);

            u32 swapChainImageCount;
            al_vk_check(vkCreateSwapchainKHR(gpu->logicalHandle, &createInfo, callbacks, &swapChain->handle));
            al_vk_check(vkGetSwapchainImagesKHR(gpu->logicalHandle, swapChain->handle, &swapChainImageCount, nullptr));
            av_construct(&swapChain->images, &bindings, swapChainImageCount);
            al_vk_check(vkGetSwapchainImagesKHR(gpu->logicalHandle, swapChain->handle, &swapChainImageCount, swapChain->images.memory));
            swapChain->format = surfaceFormat.format;
            swapChain->extent = extent;
        }
        {
            av_construct(&swapChain->imageViews, &bindings, swapChain->images.count);
            for (uSize it = 0; it < swapChain->imageViews.count; it++)
            {
                VkImageViewCreateInfo createInfo
                {
                    .sType              = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                    .pNext              = nullptr,
                    .flags              = 0,
                    .image              = swapChain->images[it],
                    .viewType           = VK_IMAGE_VIEW_TYPE_2D,
                    .format             = swapChain->format,
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
                al_vk_check(vkCreateImageView(gpu->logicalHandle, &createInfo, callbacks, &swapChain->imageViews[it]));
            }
        }
    }

    void destroy_swap_chain(SwapChain* swapChain, GPU* gpu, VkAllocationCallbacks* callbacks)
    {
        for (uSize it = 0; it < swapChain->imageViews.count; it++)
        {
            vkDestroyImageView(gpu->logicalHandle, swapChain->imageViews[it], callbacks);
        }
        vkDestroySwapchainKHR(gpu->logicalHandle, swapChain->handle, callbacks);
        av_destruct(swapChain->imageViews);
        av_destruct(swapChain->images);
    }

    void fill_image_attachment_sharing_mode(VkSharingMode* resultMode, u32* resultCount, ArrayView<u32> resultFamilyIndices, GPU* gpu)
    {
        al_vk_assert(resultFamilyIndices.count >= 2);
        if (gpu->graphicsQueue.deviceFamilyIndex == gpu->transferQueue.deviceFamilyIndex)
        {
            *resultMode = VK_SHARING_MODE_EXCLUSIVE;
            *resultCount = 0;
        }
        else
        {
            *resultMode = VK_SHARING_MODE_CONCURRENT;
            *resultCount = 2;
            resultFamilyIndices[0] = gpu->graphicsQueue.deviceFamilyIndex;
            resultFamilyIndices[1] = gpu->transferQueue.deviceFamilyIndex;
        }
    }

    VkImageType image_attachment_type_to_vk_image_type(ImageAttachment::Type type)
    {
        switch(type)
        {
            case ImageAttachment::TYPE_1D: return VK_IMAGE_TYPE_1D;
            case ImageAttachment::TYPE_2D: return VK_IMAGE_TYPE_2D;
            case ImageAttachment::TYPE_3D: return VK_IMAGE_TYPE_3D;
        }
        al_vk_assert(!"Unknown ImageAttachment::Type value");
        return VkImageType(0);
    }

    VkImageViewType image_attachment_type_to_vk_image_view_type(ImageAttachment::Type type)
    {
        switch(type)
        {
            case ImageAttachment::TYPE_1D: return VK_IMAGE_VIEW_TYPE_1D;
            case ImageAttachment::TYPE_2D: return VK_IMAGE_VIEW_TYPE_2D;
            case ImageAttachment::TYPE_3D: return VK_IMAGE_VIEW_TYPE_3D;
        }
        al_vk_assert(!"Unknown ImageAttachment::Type value");
        return VkImageViewType(0);
    }

    void construct_image_attachment(ImageAttachment* attachment, ImageAttachment::ConstructInfo constructInfo, GPU* gpu, VulkanMemoryManager* memoryManager)
    {
        attachment->extent = { constructInfo.extent.width, constructInfo.extent.height, 1 };
        attachment->format = constructInfo.format;
        {
            u32 queueFamiliesIndices[array_size(GPU::commandQueues)];
            VkImageCreateInfo createInfo
            {
                .sType                  = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .pNext                  = nullptr,
                .flags                  = 0,
                .imageType              = image_attachment_type_to_vk_image_type(constructInfo.type),
                .format                 = constructInfo.format,
                .extent                 = constructInfo.extent,
                .mipLevels              = 1,
                .arrayLayers            = 1,
                .samples                = VK_SAMPLE_COUNT_1_BIT,
                .tiling                 = VK_IMAGE_TILING_OPTIMAL,
                .usage                  = constructInfo.usage,
                .sharingMode            = VK_SHARING_MODE_EXCLUSIVE,
                .queueFamilyIndexCount  = 0,
                .pQueueFamilyIndices    = queueFamiliesIndices,
                .initialLayout          = VK_IMAGE_LAYOUT_UNDEFINED,
            };
            fill_image_attachment_sharing_mode(&createInfo.sharingMode, &createInfo.queueFamilyIndexCount, { .bindings = {}, .memory = queueFamiliesIndices, .count = array_size(queueFamiliesIndices) }, gpu);
            al_vk_check(vkCreateImage(gpu->logicalHandle, &createInfo, &memoryManager->cpu_allocationCallbacks, &attachment->image));
        }
        VkMemoryRequirements memoryRequirements;
        vkGetImageMemoryRequirements(gpu->logicalHandle, attachment->image, &memoryRequirements);
        u32 memoryTypeIndex;
        bool result = utils::get_memory_type_index(&gpu->memoryProperties, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memoryTypeIndex);
        al_vk_assert(result);
        attachment->memory = gpu_allocate(memoryManager, gpu->logicalHandle, { .sizeBytes = memoryRequirements.size, .alignment = memoryRequirements.alignment, .memoryTypeIndex = memoryTypeIndex });
        al_vk_check(vkBindImageMemory(gpu->logicalHandle, attachment->image, attachment->memory.memory, attachment->memory.offsetBytes));
        VkImageViewCreateInfo createInfo
        {
            .sType              = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext              = nullptr,
            .flags              = 0,
            .image              = attachment->image,
            .viewType           = image_attachment_type_to_vk_image_view_type(constructInfo.type),
            .format             = constructInfo.format,
            .components         = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY },
            .subresourceRange   =
            {
                .aspectMask     = constructInfo.aspect,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1,
            },
        };
        al_vk_check(vkCreateImageView(gpu->logicalHandle, &createInfo, &memoryManager->cpu_allocationCallbacks, &attachment->view));
    }

    void construct_depth_stencil_attachment(ImageAttachment* attachment, VkExtent2D extent, GPU* gpu, VulkanMemoryManager* memoryManager)
    {
        construct_image_attachment(attachment,
        {
            .extent = { extent.width, extent.height, 1 },
            .format = gpu->depthStencilFormat,
            .usage  = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            .aspect = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
            .type   = ImageAttachment::TYPE_2D,
        }, gpu, memoryManager);
    }

    void construct_color_attachment(ImageAttachment* attachment, VkExtent2D extent, GPU* gpu, VulkanMemoryManager* memoryManager)
    {
        construct_image_attachment(attachment,
        {
            .extent = { extent.width, extent.height, 1 },
            .format = VK_FORMAT_R8G8B8A8_SRGB,
            .usage  = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            .aspect = VK_IMAGE_ASPECT_COLOR_BIT,
            .type   = ImageAttachment::TYPE_2D,
        }, gpu, memoryManager);
    }

    void destroy_image_attachment(ImageAttachment* attachment, GPU* gpu, VulkanMemoryManager* memoryManager)
    {
        vkDestroyImageView(gpu->logicalHandle, attachment->view, &memoryManager->cpu_allocationCallbacks);
        vkDestroyImage(gpu->logicalHandle, attachment->image, &memoryManager->cpu_allocationCallbacks);
        gpu_deallocate(memoryManager, gpu->logicalHandle, attachment->memory);
    }

    void transition_image_layout(GPU* gpu, CommandBuffers* buffers, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
    {
        VkCommandBufferBeginInfo beginInfo
        {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext              = nullptr,
            .flags              = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            .pInheritanceInfo   = nullptr,
        };
        vkBeginCommandBuffer(buffers->primaryBuffers[0], &beginInfo);
        VkImageMemoryBarrier barrier
        {
            .sType                  = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext                  = nullptr,
            .srcAccessMask          = 0,
            .dstAccessMask          = 0,
            .oldLayout              = oldLayout,
            .newLayout              = newLayout,
            .srcQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
            .image                  = image,
            .subresourceRange       =
            {
                .aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT, // @TODO : unhardcode this
                .baseMipLevel    = 0,
                .levelCount      = 1,
                .baseArrayLayer  = 0,
                .layerCount      = 1,
            },
        };
        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;
        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            barrier.srcAccessMask   = 0;
            barrier.dstAccessMask   = VK_ACCESS_TRANSFER_WRITE_BIT;
            sourceStage             = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage        = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            barrier.srcAccessMask   = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask   = VK_ACCESS_SHADER_READ_BIT;
            sourceStage             = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage        = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else
        {
            al_vk_assert(!"Unsupported image transition");
        }
        vkCmdPipelineBarrier(buffers->primaryBuffers[0], sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        vkEndCommandBuffer(buffers->primaryBuffers[0]);
        VkSubmitInfo submitInfo
        {
            .sType                  = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext                  = nullptr,
            .waitSemaphoreCount     = 0,
            .pWaitSemaphores        = nullptr,
            .pWaitDstStageMask      = 0,
            .commandBufferCount     = 1,
            .pCommandBuffers        = &buffers->primaryBuffers[0],
            .signalSemaphoreCount   = 0,
            .pSignalSemaphores      = nullptr,
        };
        vkQueueSubmit(gpu->graphicsQueue.handle, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(gpu->graphicsQueue.handle);
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
    void tls_destroy(ThreadLocalStorage<T>* storage)
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

    void construct_command_pools(RendererBackend* backend, CommandPools* pools)
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
            .handle = createPool(backend, backend->gpu.graphicsQueue.deviceFamilyIndex),
        };
        // @TODO :  do we need to create separate pool if transfer and graphics queues are in the same family ?
        pools->transfer =
        {
            .handle = createPool(backend, backend->gpu.transferQueue.deviceFamilyIndex),
        };
    }

    void destroy_command_pools(RendererBackend* backend, CommandPools* pools)
    {
        vkDestroyCommandPool(backend->gpu.logicalHandle, pools->graphics.handle, &backend->memoryManager.cpu_allocationCallbacks);
        vkDestroyCommandPool(backend->gpu.logicalHandle, pools->transfer.handle, &backend->memoryManager.cpu_allocationCallbacks);
    }

    CommandPools* get_command_pools(RendererBackend* backend)
    {
        // CommandPools* pools = tls_access(&backend->commandPools);
        // if (!pools->graphics.handle)
        // {
        //     construct_command_pools(backend, pools);
        // }
        // return pools;
        return &backend->commandPools;
    }

    void construct_command_buffers(RendererBackend* backend, CommandBuffers* buffers)
    {
        av_construct(&buffers->primaryBuffers, &backend->memoryManager.cpu_allocationBindings, backend->swapChain.images.count);
        av_construct(&buffers->secondaryBuffers, &backend->memoryManager.cpu_allocationBindings, backend->swapChain.images.count);
        CommandPools* pools = get_command_pools(backend);
        for (uSize it = 0; it < backend->swapChain.images.count; it++)
        {
            // @TODO :  create command buffers for each possible thread
            buffers->primaryBuffers[it] = utils::create_command_buffer(backend->gpu.logicalHandle, pools->graphics.handle, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
            buffers->secondaryBuffers[it] = utils::create_command_buffer(backend->gpu.logicalHandle, pools->graphics.handle, VK_COMMAND_BUFFER_LEVEL_SECONDARY);
        }
        buffers->transferBuffer = utils::create_command_buffer(backend->gpu.logicalHandle, pools->transfer.handle, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    }

    void destroy_command_buffers(RendererBackend* backend, CommandBuffers* buffers)
    {
        av_destruct(buffers->primaryBuffers);
        av_destruct(buffers->secondaryBuffers);
    }

    void create_test_descriptor_sets(RendererBackend* backend, HardcodedDescriptorsSets* sets)
    {
        VulkanMemoryManager*    memoryManager   = &backend->memoryManager;
        SwapChain*              swapChain       = &backend->swapChain;
        GPU*                    gpu             = &backend->gpu;
        ImageAttachment*        texture         = &backend->_texture;
        VkSampler               sampler         = backend->_textureSampler;

        av_construct(&sets->buffers, &memoryManager->cpu_allocationBindings, swapChain->images.count);
        for (uSize it = 0; it < sets->buffers.count; it++)
        {
            sets->buffers[it] = create_uniform_buffer(&backend->gpu, &backend->memoryManager, sizeof(HardcodedDescriptorsSets::Ubo));
            HardcodedDescriptorsSets::Ubo ubo
            {
                { 0, 0, 1 }
            };
            copy_cpu_memory_to_buffer(gpu->logicalHandle, &ubo, &sets->buffers[it], sizeof(HardcodedDescriptorsSets::Ubo));
        }

        VkDescriptorSetLayoutBinding layoutBindings[array_size(HardcodedDescriptorsSets::BINDING_DESCS) + 1];
        for (u32 it = 0; it < array_size(HardcodedDescriptorsSets::BINDING_DESCS); it++)
        {
            layoutBindings[it] =
            {
                .binding            = it,
                .descriptorType     = HardcodedDescriptorsSets::BINDING_DESCS[it].type,
                .descriptorCount    = 1, // Size of an array of uniform buffer objects
                .stageFlags         = HardcodedDescriptorsSets::BINDING_DESCS[it].stageFlags,
                .pImmutableSamplers = nullptr,
            };
        }
        layoutBindings[array_size(HardcodedDescriptorsSets::BINDING_DESCS)] =
        {
            .binding            = array_size(HardcodedDescriptorsSets::BINDING_DESCS),
            .descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount    = 1,
            .stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr,
        };
        VkDescriptorSetLayoutCreateInfo layoutInfo
        {
            .sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext          = nullptr,
            .flags          = 0,
            .bindingCount   = array_size(layoutBindings),
            .pBindings      = layoutBindings,
        };
        al_vk_check(vkCreateDescriptorSetLayout(gpu->logicalHandle, &layoutInfo, &memoryManager->cpu_allocationCallbacks, &sets->layout));

        VkDescriptorPoolSize poolSizes[array_size(HardcodedDescriptorsSets::BINDING_DESCS) + 1];
        for (uSize it = 0; it < array_size(HardcodedDescriptorsSets::BINDING_DESCS); it++)
        {
            poolSizes[it] =
            {
                .type = HardcodedDescriptorsSets::BINDING_DESCS[it].type,
                .descriptorCount = static_cast<u32>(swapChain->images.count),
            };
        }
        poolSizes[array_size(HardcodedDescriptorsSets::BINDING_DESCS)] =
        {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = static_cast<u32>(swapChain->images.count),
        };
        VkDescriptorPoolCreateInfo poolInfo
        {
            .sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .pNext          = nullptr,
            .flags          = 0,
            .maxSets        = static_cast<u32>(swapChain->images.count),
            .poolSizeCount  = array_size(poolSizes),
            .pPoolSizes     = poolSizes,
        };
        al_vk_check(vkCreateDescriptorPool(gpu->logicalHandle, &poolInfo, &memoryManager->cpu_allocationCallbacks, &sets->pool));
        
        VkDescriptorSetLayout layouts[10]; // hack, I'm to tired to come up with "correct" (lol) solution
        for (uSize it = 0; it < swapChain->images.count; it++)
        {
            layouts[it] = sets->layout;
        }
        av_construct(&sets->sets, &memoryManager->cpu_allocationBindings, swapChain->images.count);
        VkDescriptorSetAllocateInfo allocateInfo
        {
            .sType               = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .pNext               = nullptr,
            .descriptorPool      = sets->pool,
            .descriptorSetCount  = static_cast<u32>(swapChain->images.count),
            .pSetLayouts         = layouts,
        };
        al_vk_check(vkAllocateDescriptorSets(gpu->logicalHandle, &allocateInfo, sets->sets.memory));

        for (uSize it = 0; it < swapChain->images.count; it++)
        {
            VkDescriptorBufferInfo bufferInfo
            {
                .buffer = sets->buffers[it].handle,
                .offset = 0,
                .range  = sizeof(HardcodedDescriptorsSets::Ubo), // VK_WHOLE_SIZE
            };
            VkDescriptorImageInfo imageInfo
            {
                .sampler        = sampler,
                .imageView      = texture->view,
                .imageLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };
            VkWriteDescriptorSet descriptorWrites[] =
            {
                {
                    .sType              = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .pNext              = nullptr,
                    .dstSet             = sets->sets[it],
                    .dstBinding         = 0,
                    .dstArrayElement    = 0,
                    .descriptorCount    = 1,
                    .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .pImageInfo         = nullptr,
                    .pBufferInfo        = &bufferInfo,
                    .pTexelBufferView   = nullptr,
                },
                {
                    .sType              = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .pNext              = nullptr,
                    .dstSet             = sets->sets[it],
                    .dstBinding         = 1,
                    .dstArrayElement    = 0,
                    .descriptorCount    = 1,
                    .descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .pImageInfo         = &imageInfo,
                    .pBufferInfo        = nullptr,
                    .pTexelBufferView   = nullptr,
                },
            };
            vkUpdateDescriptorSets(gpu->logicalHandle, array_size(descriptorWrites), descriptorWrites, 0, nullptr);
        }
    }

    void destroy_test_descriptor_sets(RendererBackend* backend, HardcodedDescriptorsSets* sets)
    {
        destroy_buffer(&backend->gpu, &backend->memoryManager, &sets->buffers[0]);
        destroy_buffer(&backend->gpu, &backend->memoryManager, &sets->buffers[1]);
        destroy_buffer(&backend->gpu, &backend->memoryManager, &sets->buffers[2]);
        av_destruct(sets->buffers);
        av_destruct(sets->sets);
        vkDestroyDescriptorSetLayout(backend->gpu.logicalHandle, sets->layout, &backend->memoryManager.cpu_allocationCallbacks);
        vkDestroyDescriptorPool(backend->gpu.logicalHandle, sets->pool, &backend->memoryManager.cpu_allocationCallbacks);
    }

    // void create_test_texture_image(RendererBackend* backend)
    // {
    //     copy_cpu_memory_to_buffer(backend, testEpicTexture, &backend->_stagingBuffer, sizeof(testEpicTexture));

    //     VkImageCreateInfo imageCreateInfo
    //     {
    //         .sType                  = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    //         .pNext                  = nullptr,
    //         .flags                  = 0,
    //         .imageType              = VK_IMAGE_TYPE_2D,
    //         .format                 = VK_FORMAT_R8G8B8A8_SRGB,
    //         .extent                 = { 5, 5, 1 },
    //         .mipLevels              = 1,
    //         .arrayLayers            = 1,
    //         .samples                = VK_SAMPLE_COUNT_1_BIT,
    //         .tiling                 = VK_IMAGE_TILING_OPTIMAL,
    //         .usage                  = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
    //         .sharingMode            = VK_SHARING_MODE_EXCLUSIVE,    // @TODO :  fix this. Might need to use concurrent sharing mode
    //         .queueFamilyIndexCount  = 0,                            // @TODO :  fix this. Might need to use concurrent sharing mode
    //         .pQueueFamilyIndices    = nullptr,                      // @TODO :  fix this. Might need to use concurrent sharing mode
    //         .initialLayout          = VK_IMAGE_LAYOUT_UNDEFINED,
    //     };
    //     al_vk_check(vkCreateImage(backend->gpu.logicalHandle, &imageCreateInfo, &backend->memoryManager.cpu_allocationCallbacks, &backend->testSampler.image));

    //     VkMemoryRequirements memoryRequirements;
    //     vkGetImageMemoryRequirements(backend->gpu.logicalHandle, backend->testSampler.image, &memoryRequirements);

    //     u32 memoryTypeIndex;
    //     bool result = utils::get_memory_type_index(&backend->gpu.memoryProperties, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memoryTypeIndex);
    //     al_vk_assert(result);
    //     backend->testSampler.memory = gpu_allocate(&backend->memoryManager, backend->gpu.logicalHandle, { .sizeBytes = memoryRequirements.size, .alignment = memoryRequirements.alignment, .memoryTypeIndex = memoryTypeIndex });
    //     al_vk_check(vkBindImageMemory(backend->gpu.logicalHandle, backend->testSampler.image, backend->testSampler.memory.memory, backend->testSampler.memory.offsetBytes));

    //     /*
    //     transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    //         copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
    //     transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    //     vkDestroyBuffer(device, stagingBuffer, nullptr);
    //     vkFreeMemory(device, stagingBufferMemory, nullptr);
    //     */
    // }

    // void destroy_test_texture_image(RendererBackend* backend)
    // {

    // }

    // void transition_image_layout(RendererBackend* backend, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout)
    // {
    //     CommandBuffers* buffers = get_command_buffers(backend);

    // }





























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
