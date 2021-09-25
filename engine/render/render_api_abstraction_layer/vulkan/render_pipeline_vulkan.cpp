
#include "render_pipeline_vulkan.h"
#include "render_device_vulkan.h"
#include "render_program_vulkan.h"
#include "render_pass_vulkan.h"
#include "vulkan_utils.h"

namespace al
{
    VkStencilOpState vulkan_stencil_op_state(StencilOpState* state)
    {
        return
        {
            .failOp         = utils::to_vk_stencil_op(state->failOp),
            .passOp         = utils::to_vk_stencil_op(state->passOp),
            .depthFailOp    = utils::to_vk_stencil_op(state->depthFailOp),
            .compareOp      = utils::to_vk_compare_op(state->compareOp),
            .compareMask    = state->compareMask,
            .writeMask      = state->writeMask,
            .reference      = state->reference,
        };
    }

    void vulkan_add_descriptor_pool(RenderDeviceVulkan* device, VulkanDescriptorSetLayout* layout)
    {
        VulkanDescriptorSetLayout::Pool* pool = dynamic_array_add(&layout->pools);
        al_vk_check(vkCreateDescriptorPool(device->gpu.logicalHandle, &layout->poolCreateInfo, &device->memoryManager.cpu_allocationCallbacks, &pool->handle));
        pool->isLastAllocationSuccessful = true;
        pool->numAllocations = 0;
    }

    RenderPipeline* vulkan_render_pipeline_graphics_create(GraphicsRenderPipelineCreateInfo* createInfo)
    {
        RenderDeviceVulkan* device = (RenderDeviceVulkan*)createInfo->device;
        RenderProgramVulkan* vertexProgram = (RenderProgramVulkan*)createInfo->vertexProgram;
        RenderProgramVulkan* fragmentProgram = (RenderProgramVulkan*)createInfo->fragmentProgram;
        RenderPassVulkan* renderPass = (RenderPassVulkan*)createInfo->pass;
        RenderPipelineVulkan* pipeline = allocate<RenderPipelineVulkan>(&device->memoryManager.cpu_persistentAllocator);
        VulkanSubpassInfo* subpassInfo = &renderPass->subpassInfos[createInfo->subpassIndex];
        //
        //
        //
        const bool isStencilSupported = device->gpu.flags & VulkanGpu::HAS_STENCIL;
        const bool hasVertexInput = [](const RenderProgramVulkan* program) -> bool
        {
            PointerWithSize<SpirvReflection::ShaderIO> inputs { program->reflection.shaderInputs, program->reflection.shaderInputCount };
            for (al_iterator(it, inputs)) if (!(get(it)->flags & SpirvReflection::ShaderIO::Flags::IsBuiltIn)) return true;
            return false;
        }(vertexProgram);
        al_assert_msg(!hasVertexInput, "Vertex shader inputs are not supported");
        al_assert_msg(vertexProgram->reflection.pushConstant == nullptr, "Push constants are not supported");
        al_assert_msg(fragmentProgram->reflection.pushConstant == nullptr, "Push constants are not supported");
        al_assert_msg(createInfo->subpassIndex < renderPass->subpassInfos.size, "Incorrect pipeline subpass index");
        //
        // Check that fragment shader subpass inputs and outputs match render pass settings
        //
        {
            PointerWithSize<SpirvReflection::Uniform> fragmentUniforms{ fragmentProgram->reflection.uniforms, fragmentProgram->reflection.uniformCount };
            RenderPassAttachmentRefBitMask shaderSubpassInputAttachmentMask = 0;
            for (al_iterator(it, fragmentUniforms))
            {
                SpirvReflection::Uniform* uniform = get(it);
                if (uniform->type == SpirvReflection::Uniform::InputAttachment)
                    shaderSubpassInputAttachmentMask |= RenderPassAttachmentRefBitMask(1) << uniform->inputAttachmentIndex;
            }
            al_assert_msg(shaderSubpassInputAttachmentMask == subpassInfo->inputAttachmentRefs, "Mismatch between fragment shader subpass inputs and render pass input attachments");
            PointerWithSize<SpirvReflection::ShaderIO> fragmentOutputs{ fragmentProgram->reflection.shaderOutputs, fragmentProgram->reflection.shaderOutputCount };
            RenderPassAttachmentRefBitMask shaderColorAttachmentMask = 0;
            for (al_iterator(it, fragmentOutputs))
            {
                SpirvReflection::ShaderIO* output = get(it);
                if (output->flags & SpirvReflection::ShaderIO::IsBuiltIn) continue;
                shaderColorAttachmentMask |= RenderPassAttachmentRefBitMask(1) << output->location;
            }
            al_assert_msg(shaderColorAttachmentMask == subpassInfo->colorAttachmentRefs, "Mismatch between fragment shader outputs and render pass color attachments");
        }
        //
        // Descriptor set layouts and pools
        //
        {
            RenderProgramVulkan* programs[] = { vertexProgram, fragmentProgram };
            auto combinedProgramInfo = vulkan_combined_program_info_create({ programs, array_size(programs) }, &device->memoryManager.cpu_frameAllocator);
            defer(vulkan_combined_program_info_destroy(&combinedProgramInfo));
            array_construct(&pipeline->descriptorSetLayouts, &device->memoryManager.cpu_persistentAllocator, combinedProgramInfo.descriptorSetLayoutCreateInfos.size);
            for (al_iterator(it, combinedProgramInfo.descriptorSetLayoutCreateInfos))
            {
                VkDescriptorSetLayoutCreateInfo* layoutCreateInfo = get(it);
                VulkanDescriptorSetLayout* layout = &pipeline->descriptorSetLayouts[to_index(it)];
                al_vk_check(vkCreateDescriptorSetLayout(device->gpu.logicalHandle, layoutCreateInfo, &device->memoryManager.cpu_allocationCallbacks, &layout->handle));
                uSize numUniqueDescriptorTypes = 0;
                u32 descriptorTypeMask = 0;
                for (uSize bindingIt = 0; bindingIt < layoutCreateInfo->bindingCount; bindingIt++)
                {
                    const u32 flag = [](VkDescriptorType type) -> u32
                    {
                        switch (type)
                        {
                            case VK_DESCRIPTOR_TYPE_SAMPLER:                    return u32(1) << 0;
                            case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:     return u32(1) << 1;
                            case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:              return u32(1) << 2;
                            case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:              return u32(1) << 3;
                            case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:       return u32(1) << 4;
                            case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:       return u32(1) << 5;
                            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:             return u32(1) << 6;
                            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:             return u32(1) << 7;
                            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:     return u32(1) << 8;
                            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:     return u32(1) << 9;
                            case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:           return u32(1) << 10;
                            case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT:   return u32(1) << 11;
                            case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR: return u32(1) << 12;
                            case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV:  return u32(1) << 13;
                            case VK_DESCRIPTOR_TYPE_MUTABLE_VALVE:              return u32(1) << 14;
                        };
                        al_assert("Unsupported VkDescriptorType");
                        return 0;
                    }(layoutCreateInfo->pBindings[bindingIt].descriptorType);
                    if (!(descriptorTypeMask & flag))
                    {
                        numUniqueDescriptorTypes += 1;
                        descriptorTypeMask |= flag;
                    }
                }
                array_construct(&layout->poolSizes, &device->memoryManager.cpu_persistentAllocator, numUniqueDescriptorTypes);
                for (al_iterator(poolSizeIt, layout->poolSizes)) get(poolSizeIt)->type = VK_DESCRIPTOR_TYPE_MAX_ENUM;
                for (uSize bindingIt = 0; bindingIt < layoutCreateInfo->bindingCount; bindingIt++)
                {
                    const VkDescriptorSetLayoutBinding* binding = &layoutCreateInfo->pBindings[bindingIt];
                    for (al_iterator(poolSizeIt, layout->poolSizes))
                    {
                        VkDescriptorPoolSize* poolSize = get(poolSizeIt);
                        if (poolSize->type == VK_DESCRIPTOR_TYPE_MAX_ENUM)
                        {
                            poolSize->type = binding->descriptorType;
                            poolSize->descriptorCount = binding->descriptorCount * VulkanDescriptorSetLayout::Pool::MAX_SETS;
                        }
                        else if (poolSize->type == binding->descriptorType)
                        {
                            poolSize->descriptorCount += binding->descriptorCount * VulkanDescriptorSetLayout::Pool::MAX_SETS;
                        }
                    }
                }
                layout->poolCreateInfo =
                {
                    .sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                    .pNext          = nullptr,
                    .flags          = 0,
                    .maxSets        = VulkanDescriptorSetLayout::Pool::MAX_SETS,
                    .poolSizeCount  = u32(layout->poolSizes.size),
                    .pPoolSizes     = layout->poolSizes.memory,
                };
                dynamic_array_construct(&layout->pools, &device->memoryManager.cpu_persistentAllocator);
                vulkan_add_descriptor_pool(device, layout);
            }
        }
        //
        // Pipeline layout
        //
        {
            Array<VkDescriptorSetLayout> descriptorSetLayoutHandles;
            array_construct(&descriptorSetLayoutHandles, &device->memoryManager.cpu_frameAllocator, pipeline->descriptorSetLayouts.size);
            defer(array_destruct(&descriptorSetLayoutHandles));
            for (al_iterator(handleIt, descriptorSetLayoutHandles))
            {
                *get(handleIt) = pipeline->descriptorSetLayouts[to_index(handleIt)].handle;
            }
            VkPipelineLayoutCreateInfo pipelineLayoutInfo
            {
                .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                .pNext                  = nullptr,
                .flags                  = 0,
                .setLayoutCount         = u32(descriptorSetLayoutHandles.size),
                .pSetLayouts            = descriptorSetLayoutHandles.memory,
                .pushConstantRangeCount = 0,
                .pPushConstantRanges    = nullptr,
            };
            al_vk_check(vkCreatePipelineLayout(device->gpu.logicalHandle, &pipelineLayoutInfo, &device->memoryManager.cpu_allocationCallbacks, &pipeline->layoutHandle));
        }
        //
        // Render pipeline
        //
        {
            VkPipelineShaderStageCreateInfo shaderStages[] =
            {
                utils::shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vertexProgram->handle, vertexProgram->reflection.entryPointName),
                utils::shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentProgram->handle, fragmentProgram->reflection.entryPointName),
            };
            auto vertexInputInfo = utils::vertex_input_state_create_info();
            auto inputAssembly = utils::input_assembly_state_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
            auto[viewport, scissor] = utils::default_viewport_scissor(device->swapChain.extent.width, device->swapChain.extent.height);
            auto viewportState = utils::viewport_state_create_info(&viewport, &scissor);
            auto rasterizationState = utils::rasterization_state_create_info(utils::to_vk_polygon_mode(createInfo->poligonMode), utils::to_vk_cull_mode(createInfo->cullMode), utils::to_vk_front_face(createInfo->frontFace));
            auto multisampleState = utils::multisample_state_create_info(utils::pick_sample_count(utils::to_vk_sample_count(createInfo->multisamplingType), vulkan_gpu_get_supported_framebuffer_multisample_types(&device->gpu)));
            auto frontStencilOpState = isStencilSupported && createInfo->frontStencilOpState ? vulkan_stencil_op_state(createInfo->frontStencilOpState) : VkStencilOpState{};
            auto backStencilOpState = isStencilSupported && createInfo->backStencilOpState ? vulkan_stencil_op_state(createInfo->backStencilOpState) : VkStencilOpState{};
            VkPipelineDepthStencilStateCreateInfo depthStencilState
            {
                .sType                  = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
                .pNext                  = nullptr,
                .flags                  = 0,
                .depthTestEnable        = createInfo->depthTestState ? utils::to_vk_bool(createInfo->depthTestState->isTestEnabled) : VK_FALSE,
                .depthWriteEnable       = createInfo->depthTestState ? utils::to_vk_bool(createInfo->depthTestState->isWriteEnabled) : VK_FALSE,
                .depthCompareOp         = createInfo->depthTestState ? utils::to_vk_compare_op(createInfo->depthTestState->compareOp) : VK_COMPARE_OP_ALWAYS,
                .depthBoundsTestEnable  = createInfo->depthTestState ? utils::to_vk_bool(createInfo->depthTestState->isBoundsTestEnabled) : VK_FALSE,
                .stencilTestEnable      = utils::to_vk_bool(isStencilSupported && ((createInfo->frontStencilOpState != nullptr) || (createInfo->backStencilOpState != nullptr))),
                .front                  = frontStencilOpState,
                .back                   = backStencilOpState,
                .minDepthBounds         = createInfo->depthTestState ? createInfo->depthTestState->minDepthBounds : 0.0f,
                .maxDepthBounds         = createInfo->depthTestState ? createInfo->depthTestState->maxDepthBounds : 1.0f,
            };
            VkPipelineColorBlendAttachmentState colorBlendAttachments[RenderPassCreateInfo::MAX_ATTACHMENTS] = { /* COLOR BLENDING IS UNSUPPORTED */ };
            for (al_iterator(it, colorBlendAttachments)) get(it)->colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            auto colorBlending = utils::color_blending_create_info({ colorBlendAttachments, count_bits(subpassInfo->colorAttachmentRefs) });
            auto dynamicState = utils::dynamic_state_default_create_info();
            VkGraphicsPipelineCreateInfo pipelineCreateInfo
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
                .pRasterizationState    = &rasterizationState,
                .pMultisampleState      = &multisampleState,
                .pDepthStencilState     = &depthStencilState,
                .pColorBlendState       = &colorBlending,
                .pDynamicState          = &dynamicState,
                .layout                 = pipeline->layoutHandle,
                .renderPass             = renderPass->handle,
                .subpass                = u32(createInfo->subpassIndex),
                .basePipelineHandle     = VK_NULL_HANDLE,
                .basePipelineIndex      = -1,
            };
            al_vk_check(vkCreateGraphicsPipelines(device->gpu.logicalHandle, VK_NULL_HANDLE, 1, &pipelineCreateInfo, &device->memoryManager.cpu_allocationCallbacks, &pipeline->handle));
        }
        pipeline->bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        pipeline->renderPass = renderPass;
        pipeline->device = device;

        return pipeline;
    }

    void vulkan_render_pipeline_destroy(RenderPipeline* _pipeline)
    {
        RenderPipelineVulkan* pipeline = (RenderPipelineVulkan*)_pipeline;
        RenderDeviceVulkan* device = pipeline->device;
        for (al_iterator(layoutIt, pipeline->descriptorSetLayouts))
        {
            VulkanDescriptorSetLayout* layout = get(layoutIt);
            for (al_iterator(poolIt, layout->pools))
            {
                VulkanDescriptorSetLayout::Pool* pool = get(poolIt);
                al_assert_msg(pool->numAllocations == 0, "All descriptor sets must be destroyed before the pipeline");
                vkDestroyDescriptorPool(device->gpu.logicalHandle, pool->handle, &device->memoryManager.cpu_allocationCallbacks);
            }
            dynamic_array_destruct(&layout->pools);
            array_destruct(&layout->poolSizes);
            vkDestroyDescriptorSetLayout(device->gpu.logicalHandle, layout->handle, &device->memoryManager.cpu_allocationCallbacks);
        }
        array_destruct(&pipeline->descriptorSetLayouts);
        vkDestroyPipeline(device->gpu.logicalHandle, pipeline->handle, &device->memoryManager.cpu_allocationCallbacks);
        vkDestroyPipelineLayout(device->gpu.logicalHandle, pipeline->layoutHandle, &device->memoryManager.cpu_allocationCallbacks);
        deallocate<RenderPipelineVulkan>(&pipeline->device->memoryManager.cpu_persistentAllocator, pipeline);
    }
}
