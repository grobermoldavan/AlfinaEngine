#ifndef AL_VERTEX_BUFFER_H
#define AL_VERTEX_BUFFER_H

#include <cstdint>
#include <array>

#include "engine/config/engine_config.h"

#include "render_core.h"
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

        BufferElement() noexcept;
        BufferElement(ShaderDataType type, bool isNormalized = false) noexcept;
    };

    class BufferLayout
    {
    public:
        using ElementContainer = std::array<BufferElement, EngineConfig::MAX_BUFFER_LAYOUT_ELEMENTS>;

        BufferLayout() = default;
        BufferLayout(const ElementContainer& elements) noexcept;

        const std::size_t       get_stride()        const noexcept;
        const ElementContainer& get_elements()      const noexcept;

        ElementContainer::iterator       begin()          noexcept;
        ElementContainer::iterator       end()            noexcept;
        ElementContainer::const_iterator begin()    const noexcept;
        ElementContainer::const_iterator end()      const noexcept;

    private:
        void calculate_offset_and_stride() noexcept;

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

    template<RendererType type> [[nodiscard]] VertexBuffer* create_vertex_buffer(const void* data, std::size_t size) noexcept
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

    [[nodiscard]] VertexBuffer* create_vertex_buffer(RendererType type, const void* data, std::size_t size) noexcept;
    void destroy_vertex_buffer(RendererType type, VertexBuffer* vb) noexcept;
}

#endif
