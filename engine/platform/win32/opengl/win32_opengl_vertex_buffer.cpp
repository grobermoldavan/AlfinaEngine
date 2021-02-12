
#include "win32_opengl_vertex_buffer.h"

#include "engine/memory/memory_manager.h"

namespace al::engine
{
    template<> [[nodiscard]] VertexBuffer* create_vertex_buffer<RendererType::OPEN_GL>(const void* data, std::size_t size) noexcept
    {
        VertexBuffer* vb = MemoryManager::get_pool()->allocate_as<Win32OpenglVertexBuffer>();
        ::new(vb) Win32OpenglVertexBuffer{ data, size };
        return vb;
    }

    template<> void destroy_vertex_buffer<RendererType::OPEN_GL>(VertexBuffer* vb) noexcept
    {
        vb->~VertexBuffer();
        MemoryManager::get_pool()->deallocate(reinterpret_cast<std::byte*>(vb), sizeof(Win32OpenglVertexBuffer));
    }

    Win32OpenglVertexBuffer::Win32OpenglVertexBuffer(const void* data, std::size_t size) noexcept
    {
        ::glCreateBuffers(1, &rendererId);
        ::glBindBuffer(GL_ARRAY_BUFFER, rendererId);
        ::glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
    }

    Win32OpenglVertexBuffer::~Win32OpenglVertexBuffer() noexcept
    {
        ::glDeleteBuffers(1, &rendererId);
    }

    void Win32OpenglVertexBuffer::bind() const noexcept
    {
        ::glBindBuffer(GL_ARRAY_BUFFER, rendererId);
    }

    void Win32OpenglVertexBuffer::unbind() const noexcept
    {
        ::glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void Win32OpenglVertexBuffer::set_data(const void* data, std::size_t size) noexcept
    {
        ::glBindBuffer(GL_ARRAY_BUFFER, rendererId);
        ::glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
    }

    void Win32OpenglVertexBuffer::set_layout(const BufferLayout& layout) noexcept
    {
        this->layout = layout;
    }

    const BufferLayout* Win32OpenglVertexBuffer::get_layout() const noexcept
    {
        return &layout;
    }
}
