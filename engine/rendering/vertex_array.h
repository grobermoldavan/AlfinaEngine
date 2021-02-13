#ifndef AL_VERTEX_ARRAY_H
#define AL_VERTEX_ARRAY_H

#include "vertex_buffer.h"
#include "index_buffer.h"
#include "render_core.h"
#include "engine/debug/debug.h"

namespace al::engine
{
    class VertexArray
    {
    public:
        virtual ~VertexArray() = default;

        virtual void                bind                ()                                  const   noexcept = 0;
        virtual void                unbind              ()                                  const   noexcept = 0;
        virtual void                set_vertex_buffer   (const VertexBuffer* vertexBuffer)          noexcept = 0;
        virtual void                set_index_buffer    (const IndexBuffer* indexBuffer)            noexcept = 0;
        virtual const VertexBuffer* get_vertex_buffer   ()                                  const   noexcept = 0;
        virtual const IndexBuffer*  get_index_buffer    ()                                  const   noexcept = 0;
    };

    namespace internal
    {
        template<RendererType type> [[nodiscard]] VertexArray* create_vertex_array() noexcept
        {
            al_log_error("Renderer", "Unsupported rendering API");
            al_assert(false);
            return nullptr;
        }

        template<RendererType type> void destroy_vertex_array(VertexArray* va) noexcept
        {
            al_log_error("Renderer", "Unsupported rendering API");
            al_assert(false);
        }

        [[nodiscard]] VertexArray* create_vertex_array(RendererType type) noexcept;
        void destroy_vertex_array(RendererType type, VertexArray* va) noexcept;
    }
}

#endif
