#ifndef AL_VERTEX_BUFFER_H
#define AL_VERTEX_BUFFER_H

#include <cstdint>
#include <array>

#include "engine/config/engine_config.h"

#include "renderer_type.h"
#include "engine/debug/debug.h"

namespace al::engine
{
    enum ShaderDataType : uint32_t
    {
        Float, Float2, Float3, Float4,  // floats
        Int, Int2, Int3, Int4,          // ints
        Mat3, Mat4,                     // mats
        Bool                            // bool
    };

    static const uint16_t ShaderDataTypeSize[] =
    {
        4        , 4 * 2    , 4 * 3, 4 * 4,     // floats
        4        , 4 * 2    , 4 * 3, 4 * 4,     // ints
        4 * 3 * 3, 4 * 4 * 4,                   // mats
        1                                       // bool
    };

    static const uint32_t ShaderDataTypeComponentCount[] =
    {
        1, 2, 3, 4,     // floats
        1, 2, 3, 4,     // ints
        3, 4,           // mats
        1               // bool
    };

    struct BufferElement
    {
        std::uint16_t   size;
        std::uint16_t   offset;
        ShaderDataType  type;
        bool            isNormalized;
        bool            isInitialized;

        BufferElement() noexcept
            : size{ 0 }
            , offset{ 0 }
            , isNormalized{ false }
            , isInitialized{ false }
        { }

        BufferElement(ShaderDataType type, bool isNormalized = false) noexcept
            : size          { ShaderDataTypeSize[static_cast<uint32_t>(type)] }
            , offset        { 0 }
            , type          { type }
            , isNormalized  { isNormalized }
            , isInitialized { true }
        { }
    };

    class BufferLayout
    {
    public:
        using ElementContainer = std::array<BufferElement, EngineConfig::MAX_BUFFER_LAYOUT_ELEMENTS>;

        BufferLayout() = default;

        BufferLayout(const ElementContainer& elements) noexcept
            : elements{ elements }
        {
            calculate_offset_and_stride();
        }

        const std::size_t       get_stride()        const noexcept  { return stride; }
        const ElementContainer& get_elements()      const noexcept  { return elements; }

        ElementContainer::iterator       begin()          noexcept  { return elements.begin(); }
        ElementContainer::iterator       end()            noexcept  { return elements.end(); }
        ElementContainer::const_iterator begin()    const noexcept  { return elements.begin(); }
        ElementContainer::const_iterator end()      const noexcept  { return elements.end(); }

    private:
        void calculate_offset_and_stride() noexcept
        {
            std::size_t offset = 0;
            for (auto& element : elements)
            {
                if (!element.isInitialized) { break; }
                element.offset = offset;
                offset += element.size;
            }
            stride = offset;
        }

        ElementContainer    elements;
        std::size_t         stride;
    };

    class VertexBuffer
    {
    public:
        virtual ~VertexBuffer() = default;

        virtual void                bind        ()                                      const   noexcept = 0;
        virtual void                unbind      ()                                      const   noexcept = 0;
        virtual void                set_data    (const void* data, std::size_t size)            noexcept = 0;
        virtual void                set_layout  (const BufferLayout& layout)                    noexcept = 0;
        virtual const BufferLayout* get_layout  ()                                      const   noexcept = 0;
    };

    template<RendererType type> [[nodiscard]] VertexBuffer* create_vertex_buffer(const void* data, uint32_t size) noexcept;
    template<RendererType type> void destroy_vertex_buffer(VertexBuffer* vb) noexcept;

    template<RendererType type> [[nodiscard]] VertexBuffer* create_vertex_buffer(const void* data, uint32_t size) noexcept
    {
        al_log_error("Renderer", "Unsupported rendering API");
        al_assert(false);
        return nullptr;
    }

    template<RendererType type> void destroy_vertex_buffer(VertexBuffer* vb) noexcept
    {
        al_log_error("Renderer", "Unsupported rendering API");
        al_assert(false);
    }
}

#endif
