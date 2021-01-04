
#include "index_buffer.h"

namespace al::engine
{
    [[nodiscard]] IndexBuffer* create_index_buffer(RendererType type, uint32_t* indices, std::size_t count) noexcept
    {
        switch (type)
        {
            case RendererType::OPEN_GL: return create_index_buffer<RendererType::OPEN_GL>(indices, count);
        }
        // Unknown RendererType
        al_assert(false);
        return nullptr;
    }

    void destroy_index_buffer(RendererType type, IndexBuffer* ib) noexcept
    {
        switch (type)
        {
            case RendererType::OPEN_GL: destroy_index_buffer<RendererType::OPEN_GL>(ib); return;
        }
        // Unknown RendererType
        al_assert(false);
    }
}
