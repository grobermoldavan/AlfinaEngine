
#include "renderer.h"
#include "renderer_backend_core.h"

namespace al
{
    template<typename Backend>
    void renderer_construct(Renderer<Backend>* renderer, RendererInitData* initData)
    {
        // @TODO : validate render configuration
        RendererBackendInitData backendInitData
        {
            .bindings           = initData->bindings,
            .applicationName    = "Application Name",
            .window             = initData->window,
            .renderProcessDesc  = initData->renderProcessDesc,
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

    template<typename Backend>
    void renderer_handle_resize(Renderer<Backend>* renderer)
    {
        renderer_backend_handle_resize(&renderer->backend);
    }
}

#include "vulkan/vulkan_utils.cpp"
#include "vulkan/renderer_backend_vulkan.cpp"
