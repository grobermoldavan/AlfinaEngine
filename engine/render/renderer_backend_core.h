#ifndef AL_RENDERER_BACKEND_CORE_H
#define AL_RENDERER_BACKEND_CORE_H

#include "engine/memory/memory.h"
#include "engine/platform/platform.h"
#include "engine/render/render_process_description.h"
#include "engine/math/math.h"

namespace al
{
    struct RenderVertex
    {
        f32_3 position;
        f32_3 normal;
        f32_2 uv;
    };

    struct RendererBackendInitData
    {
        AllocatorBindings           bindings;
        const char*                 applicationName;
        PlatformWindow*             window;
        RenderProcessDescription*   renderProcessDesc;
    };

    template<typename T> void renderer_backend_construct     (T* backend, RendererBackendInitData* initData)    { /* @TODO :  assert here */ }
    template<typename T> void renderer_backend_destruct      (T* backend)                                       { /* @TODO :  assert here */ }
    template<typename T> void renderer_backend_render        (T* backend)                                       { /* @TODO :  assert here */ }
    template<typename T> void renderer_backend_handle_resize (T* backend)                                       { /* @TODO :  assert here */ }
}

#endif
