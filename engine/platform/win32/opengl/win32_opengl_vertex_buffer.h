#ifndef AL_WIN32_OPENGL_VERTEX_BUFFER_H
#define AL_WIN32_OPENGL_VERTEX_BUFFER_H

#include <cstdint>

#include "win32_opengl_backend.h"

#include "engine/rendering/vertex_buffer.h"

namespace al::engine
{
    namespace internal
    {
        template<> [[nodiscard]] VertexBuffer* create_vertex_buffer<RendererType::OPEN_GL>(const void* data, std::size_t size) noexcept;
        template<> void destroy_vertex_buffer<RendererType::OPEN_GL>(VertexBuffer* vb) noexcept;
    }

    class Win32OpenglVertexBuffer : public VertexBuffer
    {
    public:
        Win32OpenglVertexBuffer(const void* data, std::size_t size) noexcept;
        ~Win32OpenglVertexBuffer() noexcept;

        virtual void                bind        ()                                      const   noexcept override;
        virtual void                unbind      ()                                      const   noexcept override;
        virtual void                set_data    (const void* data, std::size_t size)            noexcept override;
        virtual void                set_layout  (const BufferLayout& layout)                    noexcept override;
        virtual const BufferLayout* get_layout  ()                                      const   noexcept override;

    private:
        BufferLayout layout;
        RendererId rendererId;
    };
}

#endif
