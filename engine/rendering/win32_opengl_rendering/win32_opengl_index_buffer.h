#ifndef AL_WIN32_OPENGL_INDEX_BUFFER_H
#define AL_WIN32_OPENGL_INDEX_BUFFER_H

#include <cstdint>
#include <cstddef>

#include "win32_opengl_backend.h"

#include "../index_buffer.h"

namespace al::engine
{
    class Win32OpenglIndexBuffer : public IndexBuffer
    {
    public:
        Win32OpenglIndexBuffer(uint32_t* indices, std::size_t _count) noexcept;
        ~Win32OpenglIndexBuffer() noexcept;

        virtual         void            bind        ()  const noexcept override { ::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rendererId); }
        virtual         void            unbind      ()  const noexcept override { ::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); }
        virtual const   std::size_t     get_count   ()  const noexcept override { return count; }

    private:
        uint32_t rendererId;
        std::size_t count;
    };

    Win32OpenglIndexBuffer::Win32OpenglIndexBuffer(uint32_t* indices, std::size_t count) noexcept
        : count{ count }
    {
        ::glCreateBuffers(1, &rendererId);

        // Comment from https://github.com/TheCherno/Hazel/blob/master/Hazel/src/Platform/OpenGL/OpenGLBuffer.cpp :
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

#endif
