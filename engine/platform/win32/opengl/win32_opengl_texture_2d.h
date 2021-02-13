#ifndef AL_WIN32_OPENGL_TEXTURE_2D_H
#define AL_WIN32_OPENGL_TEXTURE_2D_H

#include <cstdint>

#include "win32_opengl_backend.h"

#include "engine/rendering/texture_2d.h"

namespace al::engine
{
    namespace internal
    {
        template<> [[nodiscard]] Texture2d* create_texture_2d<RendererType::OPEN_GL>(std::string_view path) noexcept;
        template<> void destroy_texture_2d<RendererType::OPEN_GL>(Texture2d* tex) noexcept;
    }

    class Win32OpenglTexure2d : public Texture2d
    {
    public:
        Win32OpenglTexure2d(std::string_view path) noexcept;
        ~Win32OpenglTexure2d() noexcept;

        virtual void bind(uint32_t slot = 0) const noexcept override;
        virtual void unbind() const noexcept override;

    private:
        GLenum internalFormat;
        GLenum dataFormat;
        uint32_t width;
        uint32_t height;
        RendererId rendererId;
    };
}

#endif
