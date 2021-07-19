
#include "vulkan_utils_converters.h"

namespace al::utils::converters
{
    VkExtent3D to_extent(u32_3 vec)
    {
        return
        {
            .width  = vec.x,
            .height = vec.y,
            .depth  = vec.z,
        };
    }

    VkFormat to_vk_format(TextureFormat textureFormat)
    {
        switch (textureFormat)
        {
            case TextureFormat::RGBA_8: return VK_FORMAT_R8G8B8A8_SRGB;
            case TextureFormat::RGBA_32F: return VK_FORMAT_R32G32B32A32_SFLOAT;
            case TextureFormat::DEPTH_STENCIL: break;
        }
        al_vk_assert(!"Unsupported or DEPTH_STENCIL TextureFormat");
        return VkFormat(0);
    }

    TextureFormat to_texture_format(VkFormat vkFormat)
    {
        switch (vkFormat)
        {
            case VK_FORMAT_R8G8B8A8_SRGB: return TextureFormat::RGBA_8;
            case VK_FORMAT_B8G8R8A8_SRGB: return TextureFormat::RGBA_8; // is this correct ?
            case VK_FORMAT_R32G32B32A32_SFLOAT: return TextureFormat::RGBA_32F;
        }
        al_vk_assert(!"Unsupported VkFormat");
        return TextureFormat(0);
    }

    VkStencilOpState to_vk_stencil_op_state(RenderStageCreateInfo::StencilOpState* stencilOpState)
    {
        auto toVkStencilOp = [](StencilOp op) -> VkStencilOp { return static_cast<VkStencilOp>(op); };
        auto toVkCompareOp = [](CompareOp op) -> VkCompareOp { return static_cast<VkCompareOp>(op); };
        return
        {
            .failOp         = toVkStencilOp(stencilOpState->failOp),
            .passOp         = toVkStencilOp(stencilOpState->passOp),
            .depthFailOp    = toVkStencilOp(stencilOpState->depthFailOp),
            .compareOp      = toVkCompareOp(stencilOpState->compareOp),
            .compareMask    = stencilOpState->compareMask,
            .writeMask      = stencilOpState->writeMask,
            .reference      = stencilOpState->reference,
        };
    }

    VkQueueFlags pipeline_stage_to_queue_flags(VkPipelineStageFlags stage)
    {
        // https://github.com/KhronosGroup/Vulkan-Docs/blob/master/chapters/synchronization.txt - synchronization-pipeline-stages-supported
        switch(stage)
        {
            case VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT:                    return ~VkQueueFlags(0);
            case VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT:                  return VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;
            case VK_PIPELINE_STAGE_VERTEX_INPUT_BIT:                   return VK_QUEUE_GRAPHICS_BIT;
            case VK_PIPELINE_STAGE_VERTEX_SHADER_BIT:                  return VK_QUEUE_GRAPHICS_BIT;
            case VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT:    return VK_QUEUE_GRAPHICS_BIT;
            case VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT: return VK_QUEUE_GRAPHICS_BIT;
            case VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT:                return VK_QUEUE_GRAPHICS_BIT;
            case VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT:                return VK_QUEUE_GRAPHICS_BIT;
            case VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT:           return VK_QUEUE_GRAPHICS_BIT;
            case VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT:            return VK_QUEUE_GRAPHICS_BIT;
            case VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT:        return VK_QUEUE_GRAPHICS_BIT;
            case VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT:                 return VK_QUEUE_COMPUTE_BIT;
            case VK_PIPELINE_STAGE_TRANSFER_BIT:                       return VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;
            case VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT:                 return ~VkQueueFlags(0);
            case VK_PIPELINE_STAGE_HOST_BIT:                           return ~VkQueueFlags(0);
            case VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT:                   return VK_QUEUE_GRAPHICS_BIT;
            case VK_PIPELINE_STAGE_ALL_COMMANDS_BIT:                   return ~VkQueueFlags(0);
        }
        return VkQueueFlags(0);
    }

    VkAccessFlags image_layout_to_access_flags(VkImageLayout layout)
    {
        switch (layout)
        {
            case VK_IMAGE_LAYOUT_UNDEFINED:                         return VkAccessFlags(0);
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:              return VK_ACCESS_TRANSFER_WRITE_BIT;
            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:          return VK_ACCESS_SHADER_READ_BIT;
            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:          return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:  return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
            case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:          return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
            case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:                   return VkAccessFlags(0); // is this correct ?
        }
        al_vk_assert(!"Unsupported VkImageLayout to VkAccessFlags conversion");
        return VkAccessFlags(0);
    }

    VkPipelineStageFlags image_layout_to_pipeline_stage_flags(VkImageLayout layout)
    {
        switch (layout)
        {
            case VK_IMAGE_LAYOUT_UNDEFINED:                         return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:              return VK_PIPELINE_STAGE_TRANSFER_BIT;
            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:          return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:          return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:  return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:          return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:                   return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT; // is this correct ?
        }
        al_vk_assert(!"Unsupported VkImageLayout to VkPipelineStageFlags conversion");
        return VkPipelineStageFlags(0);
    }
}
