#ifndef AL_WIN32_OPENGL_VERTEX_ARRAY_H
#define AL_WIN32_OPENGL_VERTEX_ARRAY_H

#include <cstdint>

#include "win32_opengl_backend.h"

#include "../vertex_array.h"
#include "engine/memory/memory_manager.h"

namespace al::engine
{
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
        uint32_t            rendererId;
        uint32_t            vertexBufferIndex;
        const VertexBuffer* vb;
        const IndexBuffer*  ib;
    };

    Win32OpenglVertexArray::Win32OpenglVertexArray() noexcept
        : vertexBufferIndex{ 0 }
        , vb{ nullptr }
        , ib{ nullptr }
    {
        ::glCreateVertexArrays(1, &rendererId);
    }

    Win32OpenglVertexArray::~Win32OpenglVertexArray() noexcept
    {
        ::glDeleteVertexArrays(1, &rendererId);
    }

    void Win32OpenglVertexArray::bind() const noexcept
    { 
        ::glBindVertexArray(rendererId);
    }

    void Win32OpenglVertexArray::unbind() const noexcept
    { 
        ::glBindVertexArray(0);
    }

    void Win32OpenglVertexArray::set_vertex_buffer(const VertexBuffer* vertexBuffer) noexcept
    { 
        static const GLenum ShaderDataTypeToOpenGlBaseType[] =
        {
            GL_FLOAT, GL_FLOAT, GL_FLOAT, GL_FLOAT, // floats
            GL_INT  , GL_INT  , GL_INT  , GL_INT,   // ints
            GL_FLOAT, GL_FLOAT,                     // mats
            GL_BOOL                                 // bool
        };

        ::glBindVertexArray(rendererId);
        vb = vertexBuffer;
        vb->bind();

        vertexBufferIndex = 0;

        const BufferLayout& layout = *vb->get_layout();
        for (const auto& element : layout)
        {
            if (!element.isInitialized) { break; }

            switch (element.type)
            {
            case ShaderDataType::Float:
            case ShaderDataType::Float2:
            case ShaderDataType::Float3:
            case ShaderDataType::Float4:
            case ShaderDataType::Int:
            case ShaderDataType::Int2:
            case ShaderDataType::Int3:
            case ShaderDataType::Int4:
            case ShaderDataType::Bool:
            {
                ::glEnableVertexAttribArray(vertexBufferIndex);
                ::glVertexAttribPointer
                (
                    vertexBufferIndex,
                    ShaderDataTypeComponentCount[element.type],
                    ShaderDataTypeToOpenGlBaseType[element.type],
                    element.isNormalized ? GL_TRUE : GL_FALSE,
                    layout.get_stride(),
                    reinterpret_cast<const void*>(element.offset)
                );

                vertexBufferIndex++;
                break;
            }
            case ShaderDataType::Mat3:
            case ShaderDataType::Mat4:
            {
                uint8_t count = ShaderDataTypeComponentCount[element.type];
                for (uint8_t it = 0; it < count; ++it)
                {
                    ::glEnableVertexAttribArray(vertexBufferIndex);
                    ::glVertexAttribPointer
                    (
                        vertexBufferIndex,
                        count,
                        ShaderDataTypeToOpenGlBaseType[element.type],
                        element.isNormalized ? GL_TRUE : GL_FALSE,
                        layout.get_stride(),
                        reinterpret_cast<const void*>(element.offset + sizeof(float) * count * it)
                    );

                    ::glVertexAttribDivisor(vertexBufferIndex, 1);
                    ++vertexBufferIndex;
                }
                break;
            }
            default:
                al_assert(false); // Unknown ShaderDataType
            }
        }
    }

    void Win32OpenglVertexArray::set_index_buffer(const IndexBuffer* indexBuffer) noexcept
    { 
        ::glBindVertexArray(rendererId);
        ib = indexBuffer;
        ib->bind();
    }

    const VertexBuffer* Win32OpenglVertexArray::get_vertex_buffer() const noexcept
    {
        return vb;
    }

    const IndexBuffer* Win32OpenglVertexArray::get_index_buffer() const noexcept
    {
        return ib;
    }

    template<> [[nodiscard]] VertexArray* create_vertex_array<RendererType::OPEN_GL>() noexcept
    {
        VertexArray* va = MemoryManager::get()->get_pool()->allocate_as<Win32OpenglVertexArray>();
        ::new(va) Win32OpenglVertexArray{ };
        return va;
    }

    template<> void destroy_vertex_array<RendererType::OPEN_GL>(VertexArray* va) noexcept
    {
        va->~VertexArray();
        MemoryManager::get()->get_pool()->deallocate(reinterpret_cast<std::byte*>(va), sizeof(Win32OpenglVertexArray));
    }
}

#endif
