
#include "vertex_array.h"

namespace al::engine
{
    namespace internal
    {
        [[nodiscard]] VertexArray* create_vertex_array(RendererType type, const VertexArrayInitData& initData) noexcept
        {
            switch (type)
            {
                case RendererType::OPEN_GL: return create_vertex_array<RendererType::OPEN_GL>(initData);
            }
            // Unknown RendererType
            al_assert(false);
            return nullptr;
        }

        void destroy_vertex_array(RendererType type, VertexArray* va) noexcept
        {
            switch (type)
            {
                case RendererType::OPEN_GL: destroy_vertex_array<RendererType::OPEN_GL>(va); return;
            }
            // Unknown RendererType
            al_assert(false);
        }
    }
}
