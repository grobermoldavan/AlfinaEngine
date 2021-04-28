#ifndef AL_RENDERER_H
#define AL_RENDERER_H

#include "render_process_description.h"
#include "renderer_backend_core.h"
#include "vulkan/renderer_backend_vulkan.h"

#include "engine/memory/memory.h"

namespace al
{
    struct RendererInitData
    {
        AllocatorBindings           bindings;
        PlatformWindow*             window;
        RenderProcessDescription*   renderProcessDesc;
    };

    template<typename Backend>
    struct Renderer
    {
        Backend backend;
    };

    template<typename Backend> void renderer_construct      (Renderer<Backend>* renderer, RendererInitData* initData);
    template<typename Backend> void renderer_destruct       (Renderer<Backend>* renderer);
    template<typename Backend> void renderer_render         (Renderer<Backend>* renderer);
    template<typename Backend> void renderer_handle_resize  (Renderer<Backend>* renderer);
}

#endif
