
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
}
