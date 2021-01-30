
#include "texture_2d.h"

namespace al::engine
{
    [[nodiscard]] Texture2d* create_texture_2d(RendererType type, std::string_view path) noexcept
    {
        switch (type)
        {
            case RendererType::OPEN_GL: return create_texture_2d<RendererType::OPEN_GL>(path);
        }
        // Unknown RendererType
        al_assert(false);
        return nullptr;
    }

    void destroy_texture_2d(RendererType type, Texture2d* tex) noexcept
    {
        switch (type)
        {
            case RendererType::OPEN_GL: destroy_texture_2d<RendererType::OPEN_GL>(tex); return;
        }
        // Unknown RendererType
        al_assert(false);
    }
}
