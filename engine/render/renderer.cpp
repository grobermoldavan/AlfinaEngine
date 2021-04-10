
#include "engine/render/renderer.h"

namespace al
{
    template<typename Backend>
    void renderer_construct(Renderer<Backend>* renderer, const RendererInitData& initData)
    {
        RendererBackendInitData backendInitData
        {
            .bindings           = initData.bindings,
            .applicationName    = "Application Name",
            .window             = initData.window,
            .isDebug            = true,
        };
        renderer_backend_construct(&renderer->backend, &backendInitData);
    }

    template<typename Backend>
    void renderer_destruct(Renderer<Backend>* renderer)
    {
        renderer_backend_destruct(&renderer->backend);
    }

    template<typename Backend>
    void renderer_render(Renderer<Backend>* renderer)
    {
        renderer_backend_render(&renderer->backend);
    }
}
