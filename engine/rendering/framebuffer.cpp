
#include "framebuffer.h"

namespace al::engine
{
    bool is_depth_attachment(FramebufferAttachmentType type)
    {
        return  (type == FramebufferAttachmentType::DEPTH_16)           || 
                (type == FramebufferAttachmentType::DEPTH_24)           || 
                (type == FramebufferAttachmentType::DEPTH_32F)          ||
                (type == FramebufferAttachmentType::DEPTH_24_STENCIL_8) ||
                (type == FramebufferAttachmentType::DEPTH_32F_STENCIL_8);
    }
}
