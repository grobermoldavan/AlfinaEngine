#ifndef AL_FRAMEBUFFER_H
#define AL_FRAMEBUFFER_H

#include <cstdint>

#include "engine/config/engine_config.h"

#include "render_core.h"
#include "engine/debug/debug.h"

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

    bool is_depth_attachment(FramebufferAttachmentType type);

    struct FramebufferDescription
    {
        ArrayContainer<FramebufferAttachmentType, EngineConfig::FRAMEBUFFER_MAX_ATTACHMENTS> attachments;
        uint32_t width;
        uint32_t height;
    };

    struct FramebufferInitData
    {
        FramebufferDescription description;
    };

    class Framebuffer
    {
    public:
        virtual ~Framebuffer() = default;

        virtual void bind       () noexcept = 0;
        virtual void unbind     () noexcept = 0;
        virtual void recreate   () noexcept = 0;
        virtual void resize     (uint32_t width, uint32_t height) noexcept = 0;

        virtual void bind_attachment_to_slot(uint32_t attachmentId, uint32_t slot) noexcept = 0;
        virtual FramebufferDescription* get_description() noexcept = 0;
        virtual const RendererId get_attachment(uint32_t attachmentId) const noexcept = 0;
    };

    namespace internal
    {
        template<RendererType type> [[nodiscard]] Framebuffer* create_framebuffer(const FramebufferInitData& initData) noexcept
        {
            al_log_error(EngineConfig::RENDERER_LOG_CATEGORY, "Unsupported rendering API");
            al_assert(false);
            return nullptr;
        }

        template<RendererType type> void destroy_framebuffer(Framebuffer* fb) noexcept
        {
            al_log_error(EngineConfig::RENDERER_LOG_CATEGORY, "Unsupported rendering API");
            al_assert(false);
        }

        [[nodiscard]] Framebuffer* create_framebuffer(RendererType type, const FramebufferInitData& initData) noexcept;
        void destroy_framebuffer(RendererType type, Framebuffer* fb) noexcept;
    }
}

#endif
