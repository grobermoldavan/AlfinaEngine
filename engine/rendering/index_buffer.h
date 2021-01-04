#ifndef AL_INDEX_BUFFER_H
#define AL_INDEX_BUFFER_H

#include <cstdint>
#include <cstddef>

#include "render_core.h"
#include "engine/debug/debug.h"

namespace al::engine
{
    class IndexBuffer
    {
    public:
        virtual ~IndexBuffer() = default;

        virtual         void            bind        ()  const noexcept = 0;
        virtual         void            unbind      ()  const noexcept = 0;
        virtual const   std::size_t     get_count   ()  const noexcept = 0;
    };

    template<RendererType type> [[nodiscard]] IndexBuffer* create_index_buffer(uint32_t* indices, std::size_t count) noexcept
    {
        al_log_error("Renderer", "Unsupported rendering API");
        al_assert(false);
        return nullptr;
    }

    template<RendererType type> void destroy_index_buffer(IndexBuffer* ib) noexcept
    {
        al_log_error("Renderer", "Unsupported rendering API");
        al_assert(false);
    }

    [[nodiscard]] IndexBuffer* create_index_buffer(RendererType type, uint32_t* indices, std::size_t count) noexcept;
    void destroy_index_buffer(RendererType type, IndexBuffer* ib) noexcept;
}

#endif
