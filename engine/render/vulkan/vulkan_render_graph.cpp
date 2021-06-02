
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
    void construct_vulkan_render_graph(VulkanRenderGraph* graph, RenderProcessDescription* renderProcessDescription, AllocatorBindings bindings)
    {
        constexpr uSize BIT_MASK_WIDTH = 32; // bitwidth of bit masks
        using BitMask = u32;
        std::memset(graph, 0, sizeof(VulkanRenderGraph));
        //
        // Get information about images used in render process
        //
        graph->imageAttachments.size = renderProcessDescription->imageAttachments.size;
        graph->imageAttachments.ptr = fixed_size_buffer_allocate<VulkanRenderGraph::ImageAttachment>(&graph->buffer, graph->imageAttachments.size);
        for (uSize it = 0; it < graph->imageAttachments.size; it++)
        {
            ImageAttachmentDescription* attachmentDesc = &renderProcessDescription->imageAttachments[it];
            VulkanRenderGraph::ImageAttachment* attachment = &graph->imageAttachments[it];
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
        graph->renderStages.size = renderProcessDescription->stages.size;
        graph->renderStages.ptr = fixed_size_buffer_allocate<VulkanRenderGraph::RenderStage>(&graph->buffer, graph->renderStages.size);
        constexpr uSize MAX_REFLECTIONS = 4;
        ArrayView<SpirvReflection> reflections;
        av_construct(&reflections, &bindings, MAX_REFLECTIONS);
        defer(av_destruct(reflections));
        //
        // Convert every RenderStageDescription to VulkanRenderGraph::RenderStage
        //
        for (uSize stageIt = 0; stageIt < renderProcessDescription->stages.size; stageIt++)
        {
            RenderStageDescription* stageDescription = &renderProcessDescription->stages[stageIt];
            VulkanRenderGraph::RenderStage* graphStage = &graph->renderStages[stageIt];
            al_vk_assert(stageDescription->shaders.size < MAX_REFLECTIONS);
            //
            // Construct reflection data for each shader of a render stage
            //
            for (uSize shaderIt = 0; shaderIt < stageDescription->shaders.size; shaderIt++)
            {
                SpirvWord* shaderBytecode = static_cast<SpirvWord*>(stageDescription->shaders[shaderIt].memory);
                uSize numWords = stageDescription->shaders[shaderIt].sizeBytes / 4;
                construct_spirv_reflection(&reflections[shaderIt], bindings, shaderBytecode, numWords);
            }
            //
            // Count the number of unique descriptor sets used within this render stage and allocate VulkanRenderGraph::DescriptorSet array
            //
            for (uSize shaderIt = 0; shaderIt < stageDescription->shaders.size; shaderIt++)
            {
                BitMask descriptorSetsMask = 0;
                SpirvReflection* reflection = &reflections[shaderIt];
                for (uSize uniformIt = 0; uniformIt < reflection->uniformCount; uniformIt++)
                {
                    SpirvReflection::Uniform* uniform = &reflection->uniforms[uniformIt];
                    al_vk_assert(uniform->set < BIT_MASK_WIDTH);
                    if (descriptorSetsMask & (1 << uniform->set)) continue;
                    descriptorSetsMask |= 1 << uniform->set;
                    graphStage->descriptorSets.size++;
                }
            }
            graphStage->descriptorSets.ptr = fixed_size_buffer_allocate<VulkanRenderGraph::DescriptorSet>(&graph->buffer, graphStage->descriptorSets.size);
            //
            // Fill descriptor sets info
            //
            uSize stageDescriptorIt = 0;
            for (uSize shaderIt = 0; shaderIt < stageDescription->shaders.size; shaderIt++)
            {
                SpirvReflection* reflection = &reflections[shaderIt];
                {
                    //
                    // Find what set values (layout(set = ...)) are used in this shader. This information will be stored in descriptorSetsMask
                    //
                    BitMask descriptorSetsMask = 0;
                    for (uSize uniformIt = 0; uniformIt < reflection->uniformCount; uniformIt++)
                    {
                        SpirvReflection::Uniform* uniform = &reflection->uniforms[uniformIt];
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
                        VulkanRenderGraph::DescriptorSet* descriptorSet = &graphStage->descriptorSets[stageDescriptorIt++];
                        BitMask descriptorBindingsMask = 0;
                        //
                        // Find what binding values (layout(..., binding = ...)) are used with this set. This information will be stored in descriptorBindingsMask
                        //
                        for (uSize uniformIt = 0; uniformIt < reflection->uniformCount; uniformIt++)
                        {
                            SpirvReflection::Uniform* uniform = &reflection->uniforms[uniformIt];
                            if (uniform->set != setMaskIt) continue;
                            al_vk_assert(uniform->binding < BIT_MASK_WIDTH);
                            al_vk_assert(((descriptorBindingsMask & (1 << uniform->binding)) == 0) && "Incorrect shader : two or more uniforms of same set has the same binding");
                            descriptorBindingsMask |= 1 << uniform->binding;
                            descriptorSet->bindings.size++;
                        }
                        descriptorSet->bindings.ptr = fixed_size_buffer_allocate<VulkanRenderGraph::DescriptorSet::Binding>(&graph->buffer, descriptorSet->bindings.size);
                        descriptorSet->set = setMaskIt;
                        uSize bindingIt = 0;
                        //
                        // Finally fill all binding information of a given descriptor set
                        //
                        for (uSize uniformIt = 0; uniformIt < reflection->uniformCount; uniformIt++)
                        {
                            SpirvReflection::Uniform* uniform = &reflection->uniforms[uniformIt];
                            if (uniform->set != setMaskIt) continue;
                            VulkanRenderGraph::DescriptorSet::Binding* binding = &descriptorSet->bindings[bindingIt];
                            *binding = VulkanRenderGraph::DescriptorSet::Binding
                            {
                                .vkDescription =
                                {
                                    .binding            = u32(uniform->binding),
                                    .descriptorType     = get_uniform_vk_descriptor_type(uniform),
                                    .descriptorCount    = 1,
                                    .stageFlags         = VkShaderStageFlags(get_shader_type_vk_stage_flag_bit(reflection->shaderType)),
                                    .pImmutableSamplers = nullptr,
                                },
                            };
                        }
                    } // for setMaskIt
                } // descriptor sets
            } // for shaderIt
        } // for stageIt












        for (uSize stageIt = 0; stageIt < renderProcessDescription->stages.size; stageIt++)
        {
            RenderStageDescription* stage = &renderProcessDescription->stages[stageIt];
            ArrayView<SpirvReflection> reflections;
            av_construct(&reflections, &bindings, stage->shaders.size);
            for (uSize shaderIt = 0; shaderIt < stage->shaders.size; shaderIt++)
            {
                SpirvWord* shaderBytecode = static_cast<SpirvWord*>(stage->shaders[shaderIt].memory);
                uSize numWords = stage->shaders[shaderIt].sizeBytes / 4;
                construct_spirv_reflection(&reflections[shaderIt], bindings, shaderBytecode, numWords);
            }
            for (uSize reflectionIt = 0; reflectionIt < reflections.count; reflectionIt++)
            {
                SpirvReflection* reflection = &reflections[reflectionIt];
                al_vk_log_msg("Shader type: %s\n", shader_type_to_str(reflection->shaderType));
                for (uSize uniformIt = 0; uniformIt < reflection->uniformCount; uniformIt++)
                {
                    SpirvReflection::Uniform* uniform = &reflection->uniforms[uniformIt];
                    al_vk_log_msg("    Uniform %s at set %d and binding %d:\n", uniform->name, uniform->set, uniform->binding);
                    if (uniform->type != SpirvReflection::Uniform::IMAGE)
                    {
                        al_vk_log_msg("        Uniform buffer size: %zd\n", get_type_info_size(uniform->buffer));
                        print_type_info(uniform->buffer, reflection, 2);
                    }
                    else
                    {
                        al_vk_log_msg("        Uniform type is image.\n");
                    }
                }
                for (uSize inputIt = 0; inputIt < reflection->shaderInputCount; inputIt++)
                {
                    SpirvReflection::ShaderIO* input = &reflection->shaderInputs[inputIt];
                    if (input->flags & (1 << SpirvReflection::ShaderIO::IS_BUILT_IN)) continue;
                    al_vk_log_msg("    Input %s of size %zd at location %d:\n", input->name, get_type_info_size(input->typeInfo), input->location);
                    print_type_info(input->typeInfo, reflection, 2);
                }
                for (uSize outputIt = 0; outputIt < reflection->shaderOutputCount; outputIt++)
                {
                    SpirvReflection::ShaderIO* output = &reflection->shaderOutputs[outputIt];
                    if (output->flags & (1 << SpirvReflection::ShaderIO::IS_BUILT_IN)) continue;
                    al_vk_log_msg("    Output %s of size %zd at location %d:\n", output->name, get_type_info_size(output->typeInfo), output->location);
                    print_type_info(output->typeInfo, reflection, 2);
                }
                if (reflection->pushConstant)
                {
                    al_vk_log_msg("    Push constant %s of size %zd:\n", reflection->pushConstant->name, get_type_info_size(reflection->pushConstant->typeInfo));
                    print_type_info(reflection->pushConstant->typeInfo, reflection, 2);
                }
            }
        }
    }
}
