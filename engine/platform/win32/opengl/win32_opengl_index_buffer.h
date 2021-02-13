#ifndef AL_WIN32_OPENGL_INDEX_BUFFER_H
#define AL_WIN32_OPENGL_INDEX_BUFFER_H

#include <cstdint>
#include <cstddef>

#include "win32_opengl_backend.h"

#include "engine/rendering/index_buffer.h"
#include "engine/memory/memory_manager.h"

namespace al::engine
{
    namespace internal
    {
        template<> [[nodiscard]] IndexBuffer* create_index_buffer<RendererType::OPEN_GL>(uint32_t* indices, std::size_t count) noexcept;
        template<> void destroy_index_buffer<RendererType::OPEN_GL>(IndexBuffer* ib) noexcept;
    }

    class Win32OpenglIndexBuffer : public IndexBuffer
    {
    public:
        Win32OpenglIndexBuffer(uint32_t* indices, std::size_t count) noexcept;
        ~Win32OpenglIndexBuffer() noexcept;

        virtual         void            bind        ()  const noexcept override;
        virtual         void            unbind      ()  const noexcept override;
        virtual const   std::size_t     get_count   ()  const noexcept override;

    private:
        RendererId rendererId;
        std::size_t count;
    };
}

#endif
