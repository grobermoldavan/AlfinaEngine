#ifndef AL_RENDERER_H
#define AL_RENDERER_H

#include "engine/memory/memory.h"
#include "engine/utilities/utilities.h"
#include "render_api_abstraction_layer/render_api_abstraction_layer.h"

namespace al
{
    struct RendererInitData
    {
        AllocatorBindings persistentAllocator;
        AllocatorBindings frameAllocator;
        PlatformWindow* window;
        RenderApi renderApi;
    };

    struct Renderer
    {
        RenderApiVtable vt;
        RenderDevice* device;
    };

    void renderer_default_construct      (Renderer* renderer, RendererInitData* initData);
    void renderer_default_destroy        (Renderer* renderer);
    void renderer_default_render         (Renderer* renderer);
    void renderer_default_handle_resize  (Renderer* renderer);
}

#endif
