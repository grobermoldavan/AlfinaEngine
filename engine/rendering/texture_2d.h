#ifndef AL_TEXTURE_2D_H
#define AL_TEXTURE_2D_H

#include "render_core.h"

namespace al::engine
{
    class Texture2d
    {
    public:
        virtual ~Texture2d() = default;

        virtual void bind(uint32_t slot = 0) const noexcept = 0;
        virtual void unbind() const noexcept = 0;
    };

    template<RendererType type> [[nodiscard]] Texture2d* create_texture_2d(const char* path) noexcept
    {
        al_log_error("Renderer", "Unsupported rendering API");
        al_assert(false);
        return nullptr;
    }

    template<RendererType type> void destroy_texture_2d(Texture2d* tex) noexcept
    {
        al_log_error("Renderer", "Unsupported rendering API");
        al_assert(false);
    }
}

#endif
