
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

    namespace internal
    {
        [[nodiscard]] Framebuffer* create_framebuffer(RendererType type, const FramebufferDescription& description) noexcept
        {
            switch (type)
            {
                case RendererType::OPEN_GL: return create_framebuffer<RendererType::OPEN_GL>(description);
            }
            // Unknown RendererType
            al_assert(false);
            return nullptr;
        }

        void destroy_framebuffer(RendererType type, Framebuffer* fb) noexcept
        {
            switch (type)
            {
                case RendererType::OPEN_GL: destroy_framebuffer<RendererType::OPEN_GL>(fb); return;
            }
            // Unknown RendererType
            al_assert(false);
        }
    }
}
