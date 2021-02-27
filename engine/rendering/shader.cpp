
#include "shader.h"

namespace al::engine
{
    namespace internal
    {
        [[nodiscard]] Shader* create_shader(RendererType type, const ShaderInitData& initData) noexcept
        {
            switch (type)
            {
                case RendererType::OPEN_GL: return create_shader<RendererType::OPEN_GL>(initData);
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
}
