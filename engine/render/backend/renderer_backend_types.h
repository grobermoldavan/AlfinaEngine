#ifndef AL_RENDERER_BACKEND_TYPES_H
#define AL_RENDERER_BACKEND_TYPES_H

namespace al
{
    enum struct RendererBackendType
    {
        VULKAN,
        OPEN_GL,
        DX_12,
        METAL,
    };
}

#endif
