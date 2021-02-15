#ifndef AL_RENDER_CORE_H
#define AL_RENDER_CORE_H

#include <cstdint>

namespace al::engine
{
    enum class RendererType
    {
        OPEN_GL,
        __end
    };

    using RendererId = uint32_t;

    struct RendererHandleT
    {
        union
        {
            struct
            {
                uint64_t isValid : 1;
                uint64_t index : 63;
            };
            uint64_t value;
        };
    };

    using RendererIndexBufferHandle     = RendererHandleT;
    using RendererVertexBufferHandle    = RendererHandleT;
    using RendererVertexArrayHandle     = RendererHandleT;
    using RendererShaderHandle          = RendererHandleT;
    using RendererFramebufferHandle     = RendererHandleT;
    using RendererTexture2dHandle       = RendererHandleT;
}

#endif
