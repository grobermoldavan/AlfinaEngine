
#include "render_stage_vulkan.h"
#include "framebuffer_vulkan.h"
#include "renderer_backend_vulkan.h"
#include "vulkan_utils.h"

namespace al
{


    ShaderProgram* vulkan_shader_program_create(RendererBackend* _backend, PlatformFile bytecode)
    {
        VulkanRendererBackend* backend = (VulkanRendererBackend*)_backend;
        VulkanShaderProgram* result = data_block_storage_add(&backend->shaderPrograms);
        Tuple<u32*, uSize> u32bytecode { (u32*)bytecode.memory, bytecode.sizeBytes };
        result->handle = utils::create_shader_module(backend->gpu.logicalHandle, u32bytecode, &backend->memoryManager.cpu_allocationCallbacks);
        return result;
    }

    void vulkan_shader_program_destroy(RendererBackend* _backend, ShaderProgram* _program)
    {
        VulkanRendererBackend* backend = (VulkanRendererBackend*)_backend;
        VulkanShaderProgram* program = (VulkanShaderProgram*)_program;
        utils::destroy_shader_module(backend->gpu.logicalHandle, program->handle, &backend->memoryManager.cpu_allocationCallbacks);
    }

    RenderStage* vulkan_render_stage_create(RendererBackend* _backend, RenderStageCreateInfo* createInfo)
    {
        VulkanRendererBackend* backend = (VulkanRendererBackend*)_backend;
        VulkanRenderStage* renderStage = data_block_storage_add(&backend->renderStages);
        const bool hasStencil = backend->gpu.flags & VulkanGpu::HAS_STENCIL;
        {
            Array<VkAttachmentDescription> attachments;
            Array<VkAttachmentReference> colorAttachmentReferences;
            uSize colorAttachmentReferenceIndex = 0;
            array_construct(&attachments, &backend->memoryManager.cpu_allocationBindings, createInfo->graphics.framebufferDescription->attachmentDescriptions.size);
            array_construct(&colorAttachmentReferences, &backend->memoryManager.cpu_allocationBindings, createInfo->graphics.framebufferDescription->attachmentDescriptions.size);
            defer(array_destruct(&attachments));
            defer(array_destruct(&colorAttachmentReferences));
            VkAttachmentReference depthAttachmentReference;
            bool hasDepthStencilAttachment = false;
            for (auto it = create_iterator(&createInfo->graphics.framebufferDescription->attachmentDescriptions); !is_finished(&it); advance(&it))
            {
                if (get(it)->format == TextureFormat::DEPTH_STENCIL)
                {
                    attachments[to_index(it)] =
                    {
                        .flags          = 0,
                        .format         = backend->gpu.depthStencilFormat,
                        .samples        = VK_SAMPLE_COUNT_1_BIT,
                        .loadOp         = VK_ATTACHMENT_LOAD_OP_LOAD,
                        .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
                        .stencilLoadOp  = hasStencil ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                        .stencilStoreOp = hasStencil ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE,
                        .initialLayout  = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                        .finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                    };
                    depthAttachmentReference =
                    {
                        .attachment = u32(to_index(it)),
                        .layout     = hasStencil ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                    };
                    hasDepthStencilAttachment = true;
                }
                else
                {
                    const bool isSwapChainImage = get(it)->flags & FramebufferDescription::AttachmentDescription::SWAP_CHAIN_ATTACHMENT;
                    attachments[to_index(it)] =
                    {
                        .flags          = 0,
                        .format         = isSwapChainImage ? backend->swapChain.surfaceFormat.format : utils::converters::to_vk_format(get(it)->format),
                        .samples        = VK_SAMPLE_COUNT_1_BIT,
                        .loadOp         = VK_ATTACHMENT_LOAD_OP_LOAD,
                        .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
                        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                        .initialLayout  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                        .finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    };
                    colorAttachmentReferences[colorAttachmentReferenceIndex++] =
                    {
                        .attachment = u32(to_index(it)),
                        .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    };
                }
            }
            VkSubpassDescription subpassDescription
            {
                .flags                      = 0,
                .pipelineBindPoint          = VK_PIPELINE_BIND_POINT_GRAPHICS,
                .inputAttachmentCount       = 0,
                .pInputAttachments          = nullptr,
                .colorAttachmentCount       = u32(colorAttachmentReferenceIndex),
                .pColorAttachments          = colorAttachmentReferences.memory,
                .pResolveAttachments        = nullptr,
                .pDepthStencilAttachment    = hasDepthStencilAttachment ? &depthAttachmentReference : nullptr,
                .preserveAttachmentCount    = 0,
                .pPreserveAttachments       = nullptr,
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
                    .dependencyFlags    = VK_DEPENDENCY_BY_REGION_BIT,
                },
                {
                    .srcSubpass         = 0,
                    .dstSubpass         = VK_SUBPASS_EXTERNAL,
                    .srcStageMask       = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                    .dstStageMask       = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                    .srcAccessMask      = 0,
                    .dstAccessMask      = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                    .dependencyFlags    = VK_DEPENDENCY_BY_REGION_BIT,
                },
            };
            VkRenderPassCreateInfo renderPassInfo
            {
                .sType              = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
                .pNext              = nullptr,
                .flags              = 0,
                .attachmentCount    = u32(attachments.size),
                .pAttachments       = attachments.memory,
                .subpassCount       = 1,
                .pSubpasses         = &subpassDescription,
                .dependencyCount    = array_size(dependencies),
                .pDependencies      = dependencies,
            };
            al_vk_check(vkCreateRenderPass(backend->gpu.logicalHandle, &renderPassInfo, &backend->memoryManager.cpu_allocationCallbacks, &renderStage->renderPass));
        }
        {
            VkPipelineShaderStageCreateInfo shaderStages[] =
            {
                {
                    .sType                  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .pNext                  = nullptr,
                    .flags                  = 0,
                    .stage                  = VK_SHADER_STAGE_VERTEX_BIT,
                    .module                 = ((VulkanShaderProgram*)createInfo->graphics.vertexShader)->handle,
                    .pName                  = "main",
                    .pSpecializationInfo    = nullptr,
                },
                {
                    .sType                  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .pNext                  = nullptr,
                    .flags                  = 0,
                    .stage                  = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .module                 = ((VulkanShaderProgram*)createInfo->graphics.fragmentShader)->handle,
                    .pName                  = "main",
                    .pSpecializationInfo    = nullptr,
                },
            };
        //     VkVertexInputBindingDescription bindingDescriptions[] =
        //     {
        //         {
        //             .binding    = 0,
        //             .stride     = sizeof(RenderVertex),
        //             .inputRate  = VK_VERTEX_INPUT_RATE_VERTEX,
        //         }
        //     };
        //     VkVertexInputAttributeDescription attributeDescriptions[]
        //     {
        //         {
        //             .location   = 0,
        //             .binding    = 0,
        //             .format     = VK_FORMAT_R32G32B32_SFLOAT,
        //             .offset     = offsetof(RenderVertex, position),
        //         },
        //         {
        //             .location   = 1,
        //             .binding    = 0,
        //             .format     = VK_FORMAT_R32G32B32_SFLOAT,
        //             .offset     = offsetof(RenderVertex, normal),
        //         },
        //         {
        //             .location   = 2,
        //             .binding    = 0,
        //             .format     = VK_FORMAT_R32G32_SFLOAT,
        //             .offset     = offsetof(RenderVertex, uv),
        //         },
        //     };
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
            VkStencilOpState stencilOpState = hasStencil ? utils::converters::to_vk_stencil_op_state(&createInfo->graphics.stencilOpState) : VkStencilOpState{};
            VkPipelineDepthStencilStateCreateInfo depthStencil
            {
                .sType                  = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
                .pNext                  = nullptr,
                .flags                  = 0,
                .depthTestEnable        = VK_TRUE,
                .depthWriteEnable       = VK_TRUE,
                .depthCompareOp         = VK_COMPARE_OP_LESS_OR_EQUAL,
                .depthBoundsTestEnable  = VK_FALSE,
                .stencilTestEnable      = VkBool32(hasStencil ? VK_TRUE : VK_FALSE),
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
        //     // Push constants can be retrieved from reflection data
        //     VkPushConstantRange testPushContantRanges[] =
        //     {
        //         {
        //             .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        //             .offset     = 0,
        //             .size       = sizeof(f32_3),
        //         }
        //     };
            VkPipelineLayoutCreateInfo pipelineLayoutInfo
            {
                .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                .pNext                  = nullptr,
                .flags                  = 0,
                .setLayoutCount         = 0,
                .pSetLayouts            = nullptr,
                .pushConstantRangeCount = 0,
                .pPushConstantRanges    = nullptr,
            };
            al_vk_check(vkCreatePipelineLayout(backend->gpu.logicalHandle, &pipelineLayoutInfo, &backend->memoryManager.cpu_allocationCallbacks, &renderStage->pipelineLayout));
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
                .layout                 = renderStage->pipelineLayout,
                .renderPass             = renderStage->renderPass,
                .subpass                = 0,
                .basePipelineHandle     = VK_NULL_HANDLE,
                .basePipelineIndex      = -1,
            };
            al_vk_check(vkCreateGraphicsPipelines(backend->gpu.logicalHandle, VK_NULL_HANDLE, 1, &pipelineInfo, &backend->memoryManager.cpu_allocationCallbacks, &renderStage->pipeline));
        }
        {
            renderStage->bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            array_construct(&renderStage->clearValues, &backend->memoryManager.cpu_allocationBindings, createInfo->graphics.framebufferDescription->attachmentDescriptions.size);
            for (auto it = create_iterator(&renderStage->clearValues); !is_finished(&it); advance(&it))
            {
                FramebufferDescription* desc = createInfo->graphics.framebufferDescription;
                FramebufferDescription::AttachmentDescription* attachment = &desc->attachmentDescriptions[to_index(it)];
                if (attachment->format == TextureFormat::DEPTH_STENCIL)
                {
                    *get(it) = VkClearValue{ .depthStencil = { 1.0f, 0 }, };
                }
                else
                {
                    *get(it) = VkClearValue{ .color = { /*all zeros*/ }, };
                }
            }
        }
        return (RenderStage*)renderStage;
    }

    void vulkan_render_stage_destroy(RendererBackend* _backend, RenderStage* _stage)
    {
        VulkanRendererBackend* backend = (VulkanRendererBackend*)_backend;
        VulkanRenderStage* stage = (VulkanRenderStage*)(_stage);
        vkDestroyRenderPass(backend->gpu.logicalHandle, stage->renderPass, &backend->memoryManager.cpu_allocationCallbacks);
        vkDestroyPipeline(backend->gpu.logicalHandle, stage->pipeline, &backend->memoryManager.cpu_allocationCallbacks);
        vkDestroyPipelineLayout(backend->gpu.logicalHandle, stage->pipelineLayout, &backend->memoryManager.cpu_allocationCallbacks);
        array_destruct(&stage->clearValues);
    }

    void vulkan_render_stage_bind(RendererBackend* _backend, RenderStage* _stage, Framebuffer* _framebuffer)
    {
        VulkanRendererBackend* backend = (VulkanRendererBackend*)_backend;
        VulkanRenderStage* stage = (VulkanRenderStage*)(_stage);
        VulkanFramebuffer* framebuffer = (VulkanFramebuffer*)(_framebuffer);
        VulkanCommandBufferSet* commandBufferSet = &backend->commandBufferSets[backend->activeSwapChainImageIndex];
        VkCommandBuffer commandBuffer = vulkan_get_command_buffer(commandBufferSet, VK_QUEUE_GRAPHICS_BIT)->handle;
        if (backend->activeRenderStage)
        {
            vkCmdEndRenderPass(commandBuffer);
            // vkQueueSubmit(get_command_queue(&backend->gpu, VulkanGpu::CommandQueue::GRAPHICS)->handle, 1, &submitInfo, backend->inFlightFences[backend->activeRenderFrame]);
        }
        for (auto it = create_iterator(&framebuffer->textures); !is_finished(&it); advance(&it))
        {
            VulkanTexture* texture = *get(it);
            if (texture->currentLayout != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
            {
                vulkan_transition_image_layout(&backend->gpu, commandBufferSet, texture->handle, &texture->subresourceRange, texture->currentLayout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            }
        }
        VkRenderPassBeginInfo renderPassInfo
        {
            .sType              = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .pNext              = nullptr,
            .renderPass         = stage->renderPass,
            .framebuffer        = framebuffer->handle,
            .renderArea         = { .offset = { 0, 0 }, .extent = backend->swapChain.extent },
            .clearValueCount    = u32(stage->clearValues.size),
            .pClearValues       = stage->clearValues.memory,
        };
        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer, stage->bindPoint, stage->pipeline);
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
            .extent = backend->swapChain.extent ,
        };
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
        vkCmdDraw(commandBuffer, 3, 1, 0 ,0);
        backend->activeRenderStage = stage;
    }
}
