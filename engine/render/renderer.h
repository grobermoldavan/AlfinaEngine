#ifndef AL_RENDERER_H
#define AL_RENDERER_H

#include "engine/memory/memory.h"
#include "engine/platform/platform.h"

namespace al
{
    struct RendererInitData
    {
        AllocatorBindings bindings;
        PlatformWindow* window;
    };

    template<typename Backend>
    struct Renderer
    {
        Backend backend;
    };

    template<typename Backend> void renderer_construct  (Renderer<Backend>* renderer, const RendererInitData& initData);
    template<typename Backend> void renderer_destruct   (Renderer<Backend>* renderer);
    template<typename Backend> void renderer_render     (Renderer<Backend>* renderer);
}

#endif
