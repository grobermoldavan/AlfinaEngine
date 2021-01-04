
#include "shader.h"

namespace al::engine
{
    [[nodiscard]] Shader* create_shader(RendererType type, const char* vertexShaderSrc, const char* fragmentShaderSrc) noexcept
    {
        switch (type)
        {
            case RendererType::OPEN_GL: return create_shader<RendererType::OPEN_GL>(vertexShaderSrc, fragmentShaderSrc);
        }
        // Unknown RendererType
        al_assert(false);
        return nullptr;
    }

    void destroy_shader(RendererType type, Shader* shader) noexcept
    {
        switch (type)
        {
            case RendererType::OPEN_GL: destroy_shader<RendererType::OPEN_GL>(shader); return;
        }
        // Unknown RendererType
        al_assert(false);
    }
}
