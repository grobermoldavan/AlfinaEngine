
#include "renderer.h"

namespace al
{
    void renderer_default_construct(Renderer* renderer, RendererInitData* initData)
    {
        render_api_vtable_fill(&renderer->vt, initData->renderApi);
        RenderDeviceCreateInfo deviceCreateInfo
        {
            .flags                  = RenderDeviceCreateInfo::IS_DEBUG,
            .window                 = initData->window,
            .persistentAllocator    = &initData->persistentAllocator,
            .frameAllocator         = &initData->frameAllocator,
        };
        renderer->device = renderer->vt.device_create(&deviceCreateInfo);
    }

    void renderer_default_destroy(Renderer* renderer)
    {
        renderer->vt.device_wait(renderer->device);
        renderer->vt.device_destroy(renderer->device);
    }

    void renderer_default_render(Renderer* renderer)
    {
    }

    void renderer_default_handle_resize(Renderer* renderer)
    {
    }
}

#include "render_api_abstraction_layer/render_api_abstraction_layer.cpp"
