
#include "win32_opengl_index_buffer.h"

#include "engine/memory/memory_manager.h"

namespace al::engine
{
    template<> [[nodiscard]] IndexBuffer* create_index_buffer<RendererType::OPEN_GL>(uint32_t* indices, std::size_t count) noexcept
    {
        IndexBuffer* ib = MemoryManager::get()->get_pool()->allocate_as<Win32OpenglIndexBuffer>();
        ::new(ib) Win32OpenglIndexBuffer{ indices, count };
        return ib;
    }

    template<> void destroy_index_buffer<RendererType::OPEN_GL>(IndexBuffer* ib) noexcept
    {
        ib->~IndexBuffer();
        MemoryManager::get()->get_pool()->deallocate(reinterpret_cast<std::byte*>(ib), sizeof(Win32OpenglIndexBuffer));
    }

    void Win32OpenglIndexBuffer::bind() const noexcept
    {
        ::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rendererId);
    }

    void Win32OpenglIndexBuffer::unbind() const noexcept
    {
        ::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    const std::size_t Win32OpenglIndexBuffer::get_count() const noexcept
    {
        return count;
    }

    Win32OpenglIndexBuffer::Win32OpenglIndexBuffer(uint32_t* indices, std::size_t count) noexcept
        : count{ count }
    {
        ::glCreateBuffers(1, &rendererId);

        // GL_ELEMENT_ARRAY_BUFFER is not valid without an actively bound VAO
        // Binding with GL_ARRAY_BUFFER allows the data to be loaded regardless of VAO state. 
        ::glBindBuffer(GL_ARRAY_BUFFER, rendererId);
        ::glBufferData(GL_ARRAY_BUFFER, count * sizeof(uint32_t), indices, GL_STATIC_DRAW);
    }

    Win32OpenglIndexBuffer::~Win32OpenglIndexBuffer() noexcept
    {
        ::glDeleteBuffers(1, &rendererId);
    }
}
