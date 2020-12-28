#ifndef AL_FRAMEBUFFER_H
#define AL_FRAMEBUFFER_H

#include <cstdint>

#include "engine/config/engine_config.h"

#include "render_core.h"

#include "utilities/array_container.h"

namespace al::engine
{
    enum class FramebufferAttachmentType : uint8_t
    {
        RGB_8,
        RGBA_8,
        RGBA_16F,
        DEPTH_16,
        DEPTH_24,
        DEPTH_32F,
        DEPTH_24_STENCIL_8,
        DEPTH_32F_STENCIL_8,
        __end
    };

    constexpr bool is_depth_attachment(FramebufferAttachmentType type)
    {
        return  (type == FramebufferAttachmentType::DEPTH_16)           || 
                (type == FramebufferAttachmentType::DEPTH_24)           || 
                (type == FramebufferAttachmentType::DEPTH_32F)          ||
                (type == FramebufferAttachmentType::DEPTH_24_STENCIL_8) ||
                (type == FramebufferAttachmentType::DEPTH_32F_STENCIL_8);
    }

    struct FramebufferDescription
    {
        ArrayContainer<FramebufferAttachmentType, EngineConfig::FRAMEBUFFER_MAX_ATTACHMENTS> attachments;
        uint32_t width;
        uint32_t height;
    };

    class Framebuffer
    {
    public:
        virtual ~Framebuffer() = default;

        virtual void bind       () noexcept = 0;
        virtual void unbind     () noexcept = 0;

        virtual void recreate   () noexcept = 0;
        virtual void resize     (uint32_t width, uint32_t height) noexcept = 0;

        virtual FramebufferDescription* get_description() = 0;
    };

    template<RendererType type> [[nodiscard]] Framebuffer* create_framebuffer(const FramebufferDescription& description) noexcept;
    template<RendererType type> void destroy_framebuffer(Framebuffer* fb) noexcept;

    template<RendererType type> [[nodiscard]] Framebuffer* create_framebuffer(const FramebufferDescription& description) noexcept
    {
        al_log_error("Renderer", "Unsupported rendering API");
        al_assert(false);
        return nullptr;
    }

    template<RendererType type> void destroy_framebuffer(Framebuffer* fb) noexcept
    {
        al_log_error("Renderer", "Unsupported rendering API");
        al_assert(false);
    }
}

#endif
