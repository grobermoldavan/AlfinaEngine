#ifndef AL_WIN32_OPENGL_VERTEX_ARRAY_H
#define AL_WIN32_OPENGL_VERTEX_ARRAY_H

#include <cstdint>

#include "win32_opengl_backend.h"

#include "engine/rendering/vertex_array.h"

namespace al::engine
{
    namespace internal
    {
        template<> [[nodiscard]] VertexArray* create_vertex_array<RendererType::OPEN_GL>(const VertexArrayInitData& initData) noexcept;
        template<> void destroy_vertex_array<RendererType::OPEN_GL>(VertexArray* va) noexcept;
    }
    
    class Win32OpenglVertexArray : public VertexArray
    {
    public:
        Win32OpenglVertexArray() noexcept;
        ~Win32OpenglVertexArray() noexcept;

        virtual void                bind                ()                                  const   noexcept override;
        virtual void                unbind              ()                                  const   noexcept override;
        virtual void                set_vertex_buffer   (const VertexBuffer* vertexBuffer)          noexcept override;
        virtual void                set_index_buffer    (const IndexBuffer* indexBuffer)            noexcept override;
        virtual const VertexBuffer* get_vertex_buffer   ()                                  const   noexcept override;
        virtual const IndexBuffer*  get_index_buffer    ()                                  const   noexcept override;

    private:
        RendererId          rendererId;
        uint32_t            vertexBufferIndex;
        const VertexBuffer* vb;
        const IndexBuffer*  ib;
    };
}

#endif
