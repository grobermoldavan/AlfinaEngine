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
}

#endif
