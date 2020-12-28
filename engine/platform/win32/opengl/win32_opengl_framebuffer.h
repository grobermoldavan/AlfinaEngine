#ifndef AL_WIN32_OPENGL_FRAMEBUFFER_H
#define AL_WIN32_OPENGL_FRAMEBUFFER_H

#include <array>

#include "win32_opengl_backend.h"

#include "engine/rendering/framebuffer.h"
#include "engine/memory/memory_manager.h"
#include "engine/debug/debug.h"

namespace al::engine
{
    class Win32OpenglFramebuffer : public Framebuffer
    {
    public:
        Win32OpenglFramebuffer(const FramebufferDescription& description);
        ~Win32OpenglFramebuffer();

        virtual void bind       () noexcept override;
        virtual void unbind     () noexcept override;
        virtual void recreate   () noexcept override;
        virtual void resize     (uint32_t width, uint32_t height) noexcept override;

        virtual FramebufferDescription* get_description() noexcept override;

    private:
        constexpr static GLint color_attachment_type_to_internal_format(FramebufferAttachmentType type)
        {
            switch (type)
            {
                case FramebufferAttachmentType::RGB_8   : return GL_RGB8;
                case FramebufferAttachmentType::RGBA_8  : return GL_RGBA8;
                case FramebufferAttachmentType::RGBA_16F: return GL_RGBA16F;
                default: al_assert(false); // Not a color attachment
            }
            return 0;
        }

        constexpr static GLenum color_attachment_type_to_format(FramebufferAttachmentType type)
        {
            switch (type)
            {
                case FramebufferAttachmentType::RGB_8   : return GL_RGB;
                case FramebufferAttachmentType::RGBA_8  : return GL_RGBA;
                case FramebufferAttachmentType::RGBA_16F: return GL_RGBA;
                default: al_assert(false); // Not a color attachment
            }
            return 0;
        }

        constexpr static GLenum color_attachment_type_to_tex_image_type(FramebufferAttachmentType type)
        {
            switch (type)
            {
                case FramebufferAttachmentType::RGB_8   : return GL_UNSIGNED_BYTE;
                case FramebufferAttachmentType::RGBA_8  : return GL_UNSIGNED_BYTE;
                case FramebufferAttachmentType::RGBA_16F: return GL_HALF_FLOAT;
                default: al_assert(false); // Not a color attachment
            }
            return 0;
        }

        constexpr static GLenum depth_attachment_type_to_internal_format(FramebufferAttachmentType type)
        {
            switch (type)
            {
                case FramebufferAttachmentType::DEPTH_16            : return GL_DEPTH_COMPONENT16;
                case FramebufferAttachmentType::DEPTH_24            : return GL_DEPTH_COMPONENT24;
                case FramebufferAttachmentType::DEPTH_32F           : return GL_DEPTH_COMPONENT32F;
                case FramebufferAttachmentType::DEPTH_24_STENCIL_8  : return GL_DEPTH24_STENCIL8;
                case FramebufferAttachmentType::DEPTH_32F_STENCIL_8 : return GL_DEPTH32F_STENCIL8;
                default: al_assert(false); // Not a depth attachment
            }
            return 0;
        }

        constexpr static GLenum depth_attachment_type_to_attachment_point(FramebufferAttachmentType type)
        {
            switch (type)
            {
                case FramebufferAttachmentType::DEPTH_16            : return GL_DEPTH_ATTACHMENT;
                case FramebufferAttachmentType::DEPTH_24            : return GL_DEPTH_ATTACHMENT;
                case FramebufferAttachmentType::DEPTH_32F           : return GL_DEPTH_ATTACHMENT;
                case FramebufferAttachmentType::DEPTH_24_STENCIL_8  : return GL_DEPTH_STENCIL_ATTACHMENT;
                case FramebufferAttachmentType::DEPTH_32F_STENCIL_8 : return GL_DEPTH_STENCIL_ATTACHMENT;
                default: al_assert(false); // Not a depth attachment
            }
            return 0;
        }

        FramebufferDescription description;
        std::array<RendererId, EngineConfig::FRAMEBUFFER_MAX_ATTACHMENTS> attachmentIds;
        RendererId rendererId;
    };

    Win32OpenglFramebuffer::Win32OpenglFramebuffer(const FramebufferDescription& description)
        : description{ description }
        , attachmentIds{ }
        , rendererId{ 0 }
    {
        recreate();
    }

    Win32OpenglFramebuffer::~Win32OpenglFramebuffer()
    {
        if (rendererId)
        {
            ::glDeleteFramebuffers(1, &rendererId);
            for (std::uint32_t it = 0; it < description.attachments.get_current_size(); it++)
            {
                ::glDeleteTextures(1, &attachmentIds[it]);
            }
        }
    }

    void Win32OpenglFramebuffer::bind() noexcept
    {
        ::glBindFramebuffer(GL_FRAMEBUFFER, rendererId);
        ::glViewport(0, 0, description.width, description.height);
    }

    void Win32OpenglFramebuffer::unbind() noexcept
    {
        ::glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void Win32OpenglFramebuffer::recreate() noexcept
    {
        if (rendererId)
        {
            ::glDeleteFramebuffers(1, &rendererId);
            for (uint32_t it = 0; it < description.attachments.get_current_size(); it++)
            {
                ::glDeleteTextures(1, &attachmentIds[it]);
            }
        }

        ::glCreateFramebuffers(1, &rendererId);
        ::glBindFramebuffer(GL_FRAMEBUFFER, rendererId);

        for (uint32_t it = 0; it < description.attachments.get_current_size(); it++)
        {
            FramebufferAttachmentType attachment = description.attachments[it];
            RendererId* attachmentId = &attachmentIds[it];
            if (is_depth_attachment(attachment))
            {
                GLenum internalFormat = depth_attachment_type_to_internal_format(attachment);
                GLenum attahcmentPoint = depth_attachment_type_to_attachment_point(attachment);

                ::glCreateTextures(GL_TEXTURE_2D, 1, attachmentId);
                ::glBindTexture(GL_TEXTURE_2D, *attachmentId);
                ::glTexStorage2D(GL_TEXTURE_2D, 1, internalFormat, description.width, description.height);
                ::glFramebufferTexture2D(GL_FRAMEBUFFER, attahcmentPoint, GL_TEXTURE_2D, *attachmentId, 0);
            }
            else
            {
                GLint internalFormat = color_attachment_type_to_internal_format(attachment);
                GLenum format = color_attachment_type_to_format(attachment);
                GLenum type = color_attachment_type_to_tex_image_type(attachment);

                ::glCreateTextures(GL_TEXTURE_2D, 1, attachmentId);
                ::glBindTexture(GL_TEXTURE_2D, *attachmentId);
                ::glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, description.width, description.height, 0, format, type, nullptr);
                ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                ::glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *attachmentId, 0);
            }
        }

        const bool isFramebufferComplete = ::glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
        if (!isFramebufferComplete)
        {
            al_log_error("OpenGL Framebuffer", "Framebuffer is not complete.");
        }

        ::glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void Win32OpenglFramebuffer::resize(uint32_t width, uint32_t height) noexcept
    {
        description.width = width;
        description.height = height;
        recreate();
    }

    FramebufferDescription* Win32OpenglFramebuffer::get_description() noexcept
    {
        return &description;
    }

    template<> [[nodiscard]] Framebuffer* create_framebuffer<RendererType::OPEN_GL>(const FramebufferDescription& description) noexcept
    {
        Framebuffer* fb = MemoryManager::get()->get_pool()->allocate_as<Win32OpenglFramebuffer>();
        ::new(fb) Win32OpenglFramebuffer{ description };
        return fb;
    }

    template<> void destroy_framebuffer<RendererType::OPEN_GL>(Framebuffer* fb) noexcept
    {
        fb->~Framebuffer();
        MemoryManager::get()->get_pool()->deallocate(reinterpret_cast<std::byte*>(fb), sizeof(Win32OpenglFramebuffer));
    }
}

#endif
