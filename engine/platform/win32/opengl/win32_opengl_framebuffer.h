#ifndef AL_WIN32_OPENGL_FRAMEBUFFER_H
#define AL_WIN32_OPENGL_FRAMEBUFFER_H

#include <cstdint>

#include "win32_opengl_backend.h"

#include "engine/rendering/framebuffer.h"
#include "engine/memory/memory_manager.h"
#include "engine/debug/debug.h"

namespace al::engine
{
    template<> [[nodiscard]] Framebuffer* create_framebuffer<RendererType::OPEN_GL>(const FramebufferDescription& description) noexcept;
    template<> void destroy_framebuffer<RendererType::OPEN_GL>(Framebuffer* fb) noexcept;

    class Win32OpenglFramebuffer : public Framebuffer
    {
    public:
        Win32OpenglFramebuffer(const FramebufferDescription& description);
        ~Win32OpenglFramebuffer();

        virtual void bind       () noexcept override;
        virtual void unbind     () noexcept override;
        virtual void recreate   () noexcept override;
        virtual void resize     (uint32_t width, uint32_t height) noexcept override;

        virtual void bind_attachment_to_slot(uint32_t attachmentId, uint32_t slot) noexcept override;
        virtual FramebufferDescription* get_description() noexcept override;
        virtual const RendererId get_attachment(uint32_t attachmentId) const noexcept override;

        GLenum get_next_color_attachment();
        void reset_color_attachment_counter();

    public:
        static GLint get_max_color_attachments();
        static GLint color_attachment_type_to_internal_format(FramebufferAttachmentType type);
        static GLenum color_attachment_type_to_format(FramebufferAttachmentType type);
        static GLenum color_attachment_type_to_tex_image_type(FramebufferAttachmentType type);
        static GLenum depth_attachment_type_to_internal_format(FramebufferAttachmentType type);
        static GLenum depth_attachment_type_to_attachment_point(FramebufferAttachmentType type);

        FramebufferDescription description;
        RendererId attachmentLocations[EngineConfig::FRAMEBUFFER_MAX_ATTACHMENTS];
        RendererId rendererId;
        uint32_t colorAttachmentsCount;
    };
}

#endif
