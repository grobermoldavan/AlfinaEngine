
#include <cstring>

#include "vulkan_render_graph.h"
#include "../spirv_reflection/spirv_reflection.h"

namespace al::vulkan
{
    static VkFormat get_image_attachment_vk_format(ImageAttachmentDescription* image)
    {
        switch(image->format)
        {
            case ImageAttachmentDescription::DEPTH_32F: return VK_FORMAT_D32_SFLOAT;
            case ImageAttachmentDescription::RGBA_8:    return VK_FORMAT_R8G8B8A8_SRGB;
            case ImageAttachmentDescription::RGB_32F:   return VK_FORMAT_R32G32B32_SFLOAT;
            default: al_vk_assert(!"Unsupported ImageAttachmentDescription::Format value");
        }
        return VkFormat(0);
    }

    static VkImageAspectFlags get_image_attachment_vk_aspect(ImageAttachmentDescription* image)
    {
        switch(image->format)
        {
            case ImageAttachmentDescription::DEPTH_32F: return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
            case ImageAttachmentDescription::RGBA_8:    return VK_IMAGE_ASPECT_COLOR_BIT;
            case ImageAttachmentDescription::RGB_32F:   return VK_IMAGE_ASPECT_COLOR_BIT;
            default: al_vk_assert(!"Unsupported ImageAttachmentDescription::Format value");
        }
        return VkImageAspectFlags(0);
    }

    static bool is_image_attachment_used_as_sampled_attachment(uSize targetAttachmentIndex, RenderProcessDescription* renderProcessDescription)
    {
        for (uSize stageIt = 0; stageIt < renderProcessDescription->stages.size; stageIt++)
        {
            RenderStageDescription* stage = &renderProcessDescription->stages[stageIt];
            for (uSize attachmentIt = 0; attachmentIt < stage->sampledAttachments.size; attachmentIt++)
            {
                if (stage->sampledAttachments[attachmentIt].imageAttachmentIndex == targetAttachmentIndex)
                {
                    return true;
                }
            }
        }
        return false;
    }

    static bool is_image_attachment_used_as_color_attachment(uSize targetAttachmentIndex, RenderProcessDescription* renderProcessDescription)
    {
        for (uSize stageIt = 0; stageIt < renderProcessDescription->stages.size; stageIt++)
        {
            RenderStageDescription* stage = &renderProcessDescription->stages[stageIt];
            for (uSize attachmentIt = 0; attachmentIt < stage->sampledAttachments.size; attachmentIt++)
            {
                if (stage->outputAttachments[attachmentIt].imageAttachmentIndex == targetAttachmentIndex)
                {
                    return true;
                }
            }
        }
        return false;
    }

    static bool is_image_attachment_used_as_depth_attachment(uSize targetAttachmentIndex, RenderProcessDescription* renderProcessDescription)
    {
        for (uSize stageIt = 0; stageIt < renderProcessDescription->stages.size; stageIt++)
        {
            RenderStageDescription* stage = &renderProcessDescription->stages[stageIt];
            if (stage->depth && stage->depth->imageAttachmentIndex == targetAttachmentIndex)
            {
                return true;
            }
        }
        return false;
    }

    static VkImageUsageFlags get_image_attachment_vk_usage(uSize targetAttachmentIndex, RenderProcessDescription* renderProcessDescription)
    {
        VkImageUsageFlags flags{};
        if (is_image_attachment_used_as_sampled_attachment(targetAttachmentIndex, renderProcessDescription)) flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
        if (is_image_attachment_used_as_color_attachment(targetAttachmentIndex, renderProcessDescription)) flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        if (is_image_attachment_used_as_depth_attachment(targetAttachmentIndex, renderProcessDescription)) flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        return flags;
    }

    static VkDescriptorType get_uniform_vk_descriptor_type(SpirvReflection::Uniform* uniform)
    {
        switch (uniform->type)
        {
            case SpirvReflection::Uniform::IMAGE:           return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            case SpirvReflection::Uniform::UNIFORM_BUFFER:  return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            case SpirvReflection::Uniform::STORAGE_BUFFER:  return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            default: al_vk_assert(!"Unsupported SpirvReflection::Uniform value");
        }
        return VkDescriptorType(0);
    }

    static VkShaderStageFlagBits get_shader_type_vk_stage_flag_bit(SpirvReflection::ShaderType shaderType)
    {
        switch (shaderType)
        {
            case SpirvReflection::ShaderType::VERTEX:   return VK_SHADER_STAGE_VERTEX_BIT;
            case SpirvReflection::ShaderType::FRAGMENT: return VK_SHADER_STAGE_FRAGMENT_BIT;
            default: al_vk_assert(!"Unsupported SpirvReflection::ShaderType value");
        }
        return VkShaderStageFlagBits(0);
    }

    //
    // This function converts user-defined RenderProcessDescription into vulkan backend-compatible VulkanRenderGraph
    //
    void construct_vulkan_render_graph(VulkanRenderGraph* renderGraph, RenderProcessDescription* renderProcessDescription, AllocatorBindings allocatorBindings)
    {
        constexpr uSize BIT_MASK_WIDTH = 64;
        using BitMask = u64;
        std::memset(renderGraph, 0, sizeof(VulkanRenderGraph));
        // ======================================================================================================
        //
        // IMAGE ATTACHMENTS
        //
        // ======================================================================================================
        renderGraph->imageAttachments.size = renderProcessDescription->imageAttachments.size;
        renderGraph->imageAttachments.ptr = fixed_size_buffer_allocate<VulkanRenderGraph::ImageAttachment>(&renderGraph->buffer, renderGraph->imageAttachments.size);
        for (uSize it = 0; it < renderGraph->imageAttachments.size; it++)
        {
            ImageAttachmentDescription* attachmentDesc = &renderProcessDescription->imageAttachments[it];
            VulkanRenderGraph::ImageAttachment* attachment = &renderGraph->imageAttachments[it];
            *attachment = VulkanRenderGraph::ImageAttachment
            {
                .extent = { attachmentDesc->width, attachmentDesc->height, 1 },
                .format = get_image_attachment_vk_format(attachmentDesc),
                .aspect = get_image_attachment_vk_aspect(attachmentDesc),
                .usage  = get_image_attachment_vk_usage(it, renderProcessDescription),
                .type   = VK_IMAGE_TYPE_2D,
            };
        }
        //
        // Allocate VulkanRenderGraph::RenderStage and SpirvReflection arrays
        //
        renderGraph->renderStages.size = renderProcessDescription->stages.size;
        renderGraph->renderStages.ptr = fixed_size_buffer_allocate<VulkanRenderGraph::RenderStage>(&renderGraph->buffer, renderGraph->renderStages.size);
        PointerWithSize<SpirvReflection> reflections
        {
            .ptr = (SpirvReflection*)allocate(&allocatorBindings, sizeof(SpirvReflection) * renderProcessDescription->shaders.size),
            .size = renderProcessDescription->shaders.size,
        };
        defer(deallocate(&allocatorBindings, reflections.ptr, sizeof(SpirvReflection) * reflections.size));
        //
        // Construct reflection data for each shader
        //
        for (uSize reflectionIt = 0; reflectionIt < reflections.size; reflectionIt++)
        {
            SpirvWord* shaderBytecode = static_cast<SpirvWord*>(renderProcessDescription->shaders[reflectionIt].memory);
            uSize numWords = renderProcessDescription->shaders[reflectionIt].sizeBytes / 4;
            construct_spirv_reflection(&reflections[reflectionIt], allocatorBindings, shaderBytecode, numWords);
        }
        //
        // Convert every RenderStageDescription to VulkanRenderGraph::RenderStage
        //
        for (uSize stageIt = 0; stageIt < renderProcessDescription->stages.size; stageIt++)
        {
            RenderStageDescription* renderStageDescription = &renderProcessDescription->stages[stageIt];
            VulkanRenderGraph::RenderStage* renderGraphStage = &renderGraph->renderStages[stageIt];
            // ======================================================================================================
            //
            // DESCRIPOR SETS
            //
            // ======================================================================================================
            {
                //
                // Count the number of unique descriptor sets used within this render stage and allocate VulkanRenderGraph::DescriptorSet array
                //
                for (uSize shaderIt = 0; shaderIt < renderStageDescription->shaders.size; shaderIt++)
                {
                    BitMask descriptorSetsMask = 0;
                    SpirvReflection* shaderReflection = &reflections[renderStageDescription->shaders[shaderIt].shaderIndex];
                    for (uSize uniformIt = 0; uniformIt < shaderReflection->uniformCount; uniformIt++)
                    {
                        SpirvReflection::Uniform* uniform = &shaderReflection->uniforms[uniformIt];
                        al_vk_assert(uniform->set < BIT_MASK_WIDTH);
                        if (descriptorSetsMask & (1 << uniform->set)) continue;
                        descriptorSetsMask |= 1 << uniform->set;
                        renderGraphStage->descriptorSets.size++;
                    }
                }
                renderGraphStage->descriptorSets.ptr = fixed_size_buffer_allocate<VulkanRenderGraph::DescriptorSet>(&renderGraph->buffer, renderGraphStage->descriptorSets.size);
                //
                // Fill descriptor sets info
                //
                uSize stageDescriptorIt = 0;
                for (uSize shaderIt = 0; shaderIt < renderStageDescription->shaders.size; shaderIt++)
                {
                    SpirvReflection* shaderReflection = &reflections[renderStageDescription->shaders[shaderIt].shaderIndex];
                    //
                    // Find what set values (layout(set = ...)) are used in this shader. This information will be stored in descriptorSetsMask
                    //
                    BitMask descriptorSetsMask = 0;
                    for (uSize uniformIt = 0; uniformIt < shaderReflection->uniformCount; uniformIt++)
                    {
                        SpirvReflection::Uniform* uniform = &shaderReflection->uniforms[uniformIt];
                        al_vk_assert(uniform->set < BIT_MASK_WIDTH);
                        if (descriptorSetsMask & (1 << uniform->set)) continue;
                        descriptorSetsMask |= 1 << uniform->set;
                    }
                    //
                    // For each used set fill information about its bindings
                    //
                    for (uSize setMaskIt = 0; setMaskIt < BIT_MASK_WIDTH; setMaskIt++)
                    {
                        if (descriptorSetsMask & (1 << setMaskIt) == 0) continue; // Skip unused sets
                        VulkanRenderGraph::DescriptorSet* renderGraphDescriptorSet = &renderGraphStage->descriptorSets[stageDescriptorIt++];
                        BitMask descriptorBindingsMask = 0;
                        //
                        // Find what binding values (layout(..., binding = ...)) are used with this set. This information will be stored in descriptorBindingsMask
                        //
                        for (uSize uniformIt = 0; uniformIt < shaderReflection->uniformCount; uniformIt++)
                        {
                            SpirvReflection::Uniform* uniform = &shaderReflection->uniforms[uniformIt];
                            if (uniform->set != setMaskIt) continue;
                            al_vk_assert(uniform->binding < BIT_MASK_WIDTH);
                            al_vk_assert(((descriptorBindingsMask & (1 << uniform->binding)) == 0) && "Incorrect shader : two or more uniforms of same set has the same binding");
                            descriptorBindingsMask |= 1 << uniform->binding;
                            renderGraphDescriptorSet->bindings.size++;
                        }
                        renderGraphDescriptorSet->bindings.ptr = fixed_size_buffer_allocate<VulkanRenderGraph::DescriptorSetBinding>(&renderGraph->buffer, renderGraphDescriptorSet->bindings.size);
                        renderGraphDescriptorSet->set = setMaskIt;
                        uSize bindingIt = 0;
                        //
                        // Finally fill all binding information of a given descriptor set
                        //
                        for (uSize uniformIt = 0; uniformIt < shaderReflection->uniformCount; uniformIt++)
                        {
                            SpirvReflection::Uniform* uniform = &shaderReflection->uniforms[uniformIt];
                            if (uniform->set != setMaskIt) continue;
                            renderGraphDescriptorSet->bindings[bindingIt++] = VulkanRenderGraph::DescriptorSetBinding
                            {
                                .binding        = uniform->binding,
                                .sizeBytes      = uniform->type == SpirvReflection::Uniform::IMAGE ? 0 : get_type_info_size(uniform->buffer),
                                .descriptorType = get_uniform_vk_descriptor_type(uniform),
                                .stageFlags     = VkShaderStageFlags(get_shader_type_vk_stage_flag_bit(shaderReflection->shaderType)),
                            };
                        }
                    } // for setMaskIt
                } // for shaderIt
            } // descriptor ses
            // ======================================================================================================
            //
            // RENDER PASS
            //
            // ======================================================================================================
            {
                uSize passAttachmentsSize = renderStageDescription->outputAttachments.size;
                if (renderStageDescription->depth) passAttachmentsSize += 1;
                renderGraphStage->renderPass.attachmentDescriptions.ptr = fixed_size_buffer_allocate<VkAttachmentDescription>(&renderGraph->buffer, passAttachmentsSize);
                renderGraphStage->renderPass.attachmentDescriptions.size = passAttachmentsSize;
                //
                // Construct VkAttachmentDescription for output and depth attachments
                //
                for (uSize outputAttachmentIt = 0; outputAttachmentIt < renderStageDescription->outputAttachments.size; outputAttachmentIt++)
                {
                    VkAttachmentDescription* attachment = &renderGraphStage->renderPass.attachmentDescriptions[outputAttachmentIt];
                    uSize attachmentDescriptionIndex = renderStageDescription->outputAttachments[outputAttachmentIt].imageAttachmentIndex;
                    ImageAttachmentDescription* attachmentDescription = &renderProcessDescription->imageAttachments[attachmentDescriptionIndex];
                    bool wasWrittenBefore = false;
                    for (uSize innerStageIt = 0; innerStageIt < stageIt; innerStageIt++)
                    {
                        RenderStageDescription* innerStageDescription = &renderProcessDescription->stages[innerStageIt];
                        for (uSize innerOutputAttachmentIt = 0; innerOutputAttachmentIt < innerStageDescription->outputAttachments.size; innerOutputAttachmentIt++)
                        {
                            uSize innerOutputAttachmentIndex = innerStageDescription->outputAttachments[innerOutputAttachmentIt].imageAttachmentIndex;
                            if (innerOutputAttachmentIndex == attachmentDescriptionIndex)
                            {
                                wasWrittenBefore = true;
                                break;
                            }
                        }
                        if (wasWrittenBefore) break;
                    }
                    *attachment = VkAttachmentDescription
                    {
                        .flags          = 0,
                        .format         = get_image_attachment_vk_format(attachmentDescription),
                        .samples        = VK_SAMPLE_COUNT_1_BIT,
                        .loadOp         = wasWrittenBefore ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR,
                        .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
                        .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                        .initialLayout  = wasWrittenBefore ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED,
                        .finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    };
                }
                if (renderStageDescription->depth)
                {
                    bool wasWrittenBefore = false;
                    for (uSize innerStageIt = 0; innerStageIt < stageIt; innerStageIt++)
                    {
                        RenderStageDescription* innerStageDescription = &renderProcessDescription->stages[innerStageIt];
                        if (!innerStageDescription->depth) continue;
                        if (innerStageDescription->depth->imageAttachmentIndex == renderStageDescription->depth->imageAttachmentIndex)
                        {
                            wasWrittenBefore = true;
                            break;
                        }
                    }
                    renderGraphStage->renderPass.attachmentDescriptions[renderGraphStage->renderPass.attachmentDescriptions.size - 1] = VkAttachmentDescription
                    {
                        .flags          = 0,
                        .format         = get_image_attachment_vk_format(&renderProcessDescription->imageAttachments[renderStageDescription->depth->imageAttachmentIndex]),
                        .samples        = VK_SAMPLE_COUNT_1_BIT,
                        .loadOp         = wasWrittenBefore ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR,
                        .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
                        .stencilLoadOp  = wasWrittenBefore ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR,
                        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE,
                        .initialLayout  = wasWrittenBefore ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED,
                        .finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                    };
                }
                //
                // Construct VkAttachmentReference for color and depth attachments
                //
                uSize colorAttachmentReferencesSize = renderStageDescription->outputAttachments.size;
                renderGraphStage->renderPass.colorAttachmentReferences.ptr = fixed_size_buffer_allocate<VkAttachmentReference>(&renderGraph->buffer, colorAttachmentReferencesSize);
                renderGraphStage->renderPass.colorAttachmentReferences.size = colorAttachmentReferencesSize;
                for (uSize colorAttachmentReferenceIt = 0; colorAttachmentReferenceIt < colorAttachmentReferencesSize; colorAttachmentReferenceIt++)
                {
                    VkAttachmentReference* colorAttachmentReference = &renderGraphStage->renderPass.colorAttachmentReferences[colorAttachmentReferenceIt];
                    *colorAttachmentReference = VkAttachmentReference
                    {
                        .attachment = u32(colorAttachmentReferenceIt),
                        .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    };
                }
                if (renderStageDescription->depth)
                {
                    renderGraphStage->renderPass.depthAttachmentReference = fixed_size_buffer_allocate<VkAttachmentReference>(&renderGraph->buffer);
                    *renderGraphStage->renderPass.depthAttachmentReference = VkAttachmentReference
                    {
                        .attachment = u32(renderGraphStage->renderPass.attachmentDescriptions.size - 1),
                        .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    };
                }
                //
                // Construct subpass description
                //
                renderGraphStage->renderPass.subpassDescription = VkSubpassDescription
                {
                    .flags                      = 0,
                    .pipelineBindPoint          = VK_PIPELINE_BIND_POINT_GRAPHICS,
                    .inputAttachmentCount       = 0,
                    .pInputAttachments          = nullptr,
                    .colorAttachmentCount       = u32(renderGraphStage->renderPass.colorAttachmentReferences.size),
                    .pColorAttachments          = renderGraphStage->renderPass.colorAttachmentReferences.ptr,
                    .pResolveAttachments        = nullptr,
                    .pDepthStencilAttachment    = renderGraphStage->renderPass.depthAttachmentReference,
                    .preserveAttachmentCount    = 0,
                    .pPreserveAttachments       = nullptr,
                };
                //
                // Construct subpass dependencies
                //
                renderGraphStage->renderPass.subpassDependencies[0] = VkSubpassDependency
                {
                    .srcSubpass         = VK_SUBPASS_EXTERNAL,
                    .dstSubpass         = 0,
                    .srcStageMask       = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                    .dstStageMask       = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                    .srcAccessMask      = VK_ACCESS_MEMORY_WRITE_BIT,
                    .dstAccessMask      = VK_ACCESS_MEMORY_READ_BIT,
                    .dependencyFlags    = VK_DEPENDENCY_BY_REGION_BIT,
                };
                renderGraphStage->renderPass.subpassDependencies[1] = VkSubpassDependency
                {
                    .srcSubpass         = 0,
                    .dstSubpass         = VK_SUBPASS_EXTERNAL,
                    .srcStageMask       = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                    .dstStageMask       = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                    .srcAccessMask      = VK_ACCESS_MEMORY_WRITE_BIT,
                    .dstAccessMask      = VK_ACCESS_MEMORY_READ_BIT,
                    .dependencyFlags    = VK_DEPENDENCY_BY_REGION_BIT,
                };
                //
                // Construct create info
                //
                renderGraphStage->renderPass.createInfo = VkRenderPassCreateInfo
                {
                    .sType              = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
                    .pNext              = nullptr,
                    .flags              = 0,
                    .attachmentCount    = u32(renderGraphStage->renderPass.attachmentDescriptions.size),
                    .pAttachments       = renderGraphStage->renderPass.attachmentDescriptions.ptr,
                    .subpassCount       = 1,
                    .pSubpasses         = &renderGraphStage->renderPass.subpassDescription,
                    .dependencyCount    = 2,
                    .pDependencies      = renderGraphStage->renderPass.subpassDependencies,
                };
            } // render pass
            // ======================================================================================================
            //
            // FRAMEBUFFER
            //
            // ======================================================================================================
            {
                renderGraphStage->framebuffer = VulkanRenderGraph::Framebuffer
                {
                    .imageAttachmentReferences =
                    {
                        .ptr = fixed_size_buffer_allocate<uSize>(&renderGraph->buffer, renderGraphStage->renderPass.attachmentDescriptions.size),
                        .size = renderGraphStage->renderPass.attachmentDescriptions.size,
                    },
                };
                uSize attachmentReferenceIt = 0;
                for (uSize outputAttachmentIt = 0; outputAttachmentIt < renderStageDescription->outputAttachments.size; outputAttachmentIt++)
                {
                    uSize attachmentDescriptionIndex = renderStageDescription->outputAttachments[outputAttachmentIt].imageAttachmentIndex;
                    renderGraphStage->framebuffer.imageAttachmentReferences[attachmentReferenceIt++] = attachmentDescriptionIndex;
                }
                if (renderStageDescription->depth)
                {
                    renderGraphStage->framebuffer.imageAttachmentReferences[attachmentReferenceIt++] = renderStageDescription->depth->imageAttachmentIndex;
                }
                // @NOTE : framebuffer size will be determined in the runtime (because it might be dependent on window size)
            } // framebuffer
            // ======================================================================================================
            //
            // RENDER PIPELINE
            //
            // ======================================================================================================
            {
                //
                // Shader stages
                //
                VulkanRenderGraph::RenderPipeline* renderGraphPipeline = &renderGraphStage->renderPipeline;
                renderGraphPipeline->shaderStages.ptr = fixed_size_buffer_allocate<VulkanRenderGraph::RenderPipelineShaderStage>(&renderGraph->buffer, renderStageDescription->shaders.size);
                renderGraphPipeline->shaderStages.size = renderStageDescription->shaders.size;
                for (uSize shaderStageIt = 0; shaderStageIt < renderGraphPipeline->shaderStages.size; shaderStageIt++)
                {
                    uSize shaderIndex = renderStageDescription->shaders[shaderStageIt].shaderIndex;
                    renderGraphPipeline->shaderStages[shaderStageIt] = VulkanRenderGraph::RenderPipelineShaderStage
                    {
                        .bytecode   = renderProcessDescription->shaders[shaderIndex],
                        .stage      = get_shader_type_vk_stage_flag_bit(reflections[shaderIndex].shaderType),
                        .entryPoint = reflections[shaderIndex].entryPointName,
                    };
                    if (reflections[shaderIndex].pushConstant)
                    {
                        renderGraphPipeline->pushConstants.size += 1;
                    }
                }
                //
                // Push constants
                //
                // @TODO : support push constants!
                // @TODO : support push constants!
                // @TODO : support push constants!
                // @TODO : support push constants!
                // @TODO : support push constants!
                // if (renderGraphPipeline->pushConstants.size > 0)
                // {
                //     renderGraphPipeline->pushConstants.ptr = fixed_size_buffer_allocate<VkPushConstantRange>(&renderGraph->buffer, renderGraphPipeline->pushConstants.size);
                // }
                // uSize pushConstantIt = 0;
                // uSize offset = 0;
                // for (uSize shaderStageIt = 0; shaderStageIt < renderGraphPipeline->shaderStages.size; shaderStageIt++)
                // {
                //     uSize shaderIndex = renderStageDescription->shaders[shaderStageIt].shaderIndex;
                //     if (!reflections[shaderIndex].pushConstant)
                //     {
                //         continue;
                //     }
                //     // @NOTE : this code might not work correct if we use the same push constant in the defferent shader stages. Will need to rework this
                //     // @NOTE : this code might not work correct if we use the same push constant in the defferent shader stages. Will need to rework this
                //     // @NOTE : this code might not work correct if we use the same push constant in the defferent shader stages. Will need to rework this
                //     uSize pushConstantSize = get_type_info_size(reflections[shaderIndex].pushConstant->typeInfo);
                //     renderGraphPipeline->pushConstants[pushConstantIt++] = VkPushConstantRange
                //     {
                //         .stageFlags = VkShaderStageFlags(get_shader_type_vk_stage_flag_bit(reflections[shaderIndex].shaderType)),
                //         .offset     = u32(offset),
                //         .size       = u32(pushConstantSize),
                //     };
                //     offset += pushConstantSize;
                // }
            } // render pipeline
            // ======================================================================================================
            //
            // STAGE DEPENDENCIES
            //
            // ======================================================================================================
            {
                if (renderStageDescription->stageDependencies.size)
                {
                    renderGraphStage->stageDependencies.size = renderStageDescription->stageDependencies.size;
                    renderGraphStage->stageDependencies.ptr = fixed_size_buffer_allocate<uSize>(&renderGraph->buffer, renderGraphStage->stageDependencies.size);
                }
                for (uSize dependencyIt = 0; dependencyIt < renderStageDescription->stageDependencies.size; dependencyIt++)
                {
                    renderGraphStage->stageDependencies[dependencyIt] = renderStageDescription->stageDependencies[dependencyIt];
                }
            }
        } // for stageIt


















        // for (uSize stageIt = 0; stageIt < renderProcessDescription->stages.size; stageIt++)
        // {
        //     RenderStageDescription* stage = &renderProcessDescription->stages[stageIt];
        //     ArrayView<SpirvReflection> reflections;
        //     av_construct(&reflections, &allocatorBindings, stage->shaders.size);
        //     for (uSize shaderIt = 0; shaderIt < stage->shaders.size; shaderIt++)
        //     {
        //         SpirvWord* shaderBytecode = static_cast<SpirvWord*>(renderProcessDescription->shaders[stage->shaders[shaderIt].shaderIndex].memory);
        //         uSize numWords = renderProcessDescription->shaders[stage->shaders[shaderIt].shaderIndex].sizeBytes / 4;
        //         construct_spirv_reflection(&reflections[shaderIt], allocatorBindings, shaderBytecode, numWords);
        //     }
        //     for (uSize reflectionIt = 0; reflectionIt < reflections.count; reflectionIt++)
        //     {
        //         SpirvReflection* reflection = &reflections[reflectionIt];
        //         al_vk_log_msg("Shader type: %s\n", shader_type_to_str(reflection->shaderType));
        //         for (uSize uniformIt = 0; uniformIt < reflection->uniformCount; uniformIt++)
        //         {
        //             SpirvReflection::Uniform* uniform = &reflection->uniforms[uniformIt];
        //             al_vk_log_msg("    Uniform %s at set %d and binding %d:\n", uniform->name, uniform->set, uniform->binding);
        //             if (uniform->type != SpirvReflection::Uniform::IMAGE)
        //             {
        //                 al_vk_log_msg("        Uniform buffer size: %zd\n", get_type_info_size(uniform->buffer));
        //                 print_type_info(uniform->buffer, reflection, 2);
        //             }
        //             else
        //             {
        //                 al_vk_log_msg("        Uniform type is image.\n");
        //             }
        //         }
        //         for (uSize inputIt = 0; inputIt < reflection->shaderInputCount; inputIt++)
        //         {
        //             SpirvReflection::ShaderIO* input = &reflection->shaderInputs[inputIt];
        //             if (input->flags & (1 << SpirvReflection::ShaderIO::IS_BUILT_IN)) continue;
        //             al_vk_log_msg("    Input %s of size %zd at location %d:\n", input->name, get_type_info_size(input->typeInfo), input->location);
        //             print_type_info(input->typeInfo, reflection, 2);
        //         }
        //         for (uSize outputIt = 0; outputIt < reflection->shaderOutputCount; outputIt++)
        //         {
        //             SpirvReflection::ShaderIO* output = &reflection->shaderOutputs[outputIt];
        //             if (output->flags & (1 << SpirvReflection::ShaderIO::IS_BUILT_IN)) continue;
        //             al_vk_log_msg("    Output %s of size %zd at location %d:\n", output->name, get_type_info_size(output->typeInfo), output->location);
        //             print_type_info(output->typeInfo, reflection, 2);
        //         }
        //         if (reflection->pushConstant)
        //         {
        //             al_vk_log_msg("    Push constant %s of size %zd:\n", reflection->pushConstant->name, get_type_info_size(reflection->pushConstant->typeInfo));
        //             print_type_info(reflection->pushConstant->typeInfo, reflection, 2);
        //         }
        //     }
        // }
    }
}
