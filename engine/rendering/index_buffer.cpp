
#include "index_buffer.h"

namespace al::engine
{
    namespace internal
    {
        [[nodiscard]] IndexBuffer* create_index_buffer(RendererType type, const IndexBufferInitData& initData) noexcept
        {
            switch (type)
            {
                case RendererType::OPEN_GL: return create_index_buffer<RendererType::OPEN_GL>(initData);
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
}
