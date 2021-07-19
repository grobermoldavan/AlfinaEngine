#ifndef AL_VULKAN_UTILS_CONVERTERS_H
#define AL_VULKAN_UTILS_CONVERTERS_H

#include "engine/types.h"
#include "engine/math/math.h"
#include "vulkan_base.h"
#include "../texture_formats.h"
#include "../render_stage_base.h"

namespace al::utils::converters
{
    VkExtent3D to_extent(u32_3 vec);
    VkFormat to_vk_format(TextureFormat textureFormat);
    TextureFormat to_texture_format(VkFormat vkFormat);
    VkStencilOpState to_vk_stencil_op_state(RenderStageCreateInfo::StencilOpState* stencilOpState);
    VkQueueFlags pipeline_stage_to_queue_flags(VkPipelineStageFlags stage);
    VkAccessFlags image_layout_to_access_flags(VkImageLayout layout);
    VkPipelineStageFlags image_layout_to_pipeline_stage_flags(VkImageLayout layout);
}

#endif
