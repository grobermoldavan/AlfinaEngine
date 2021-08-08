#ifndef AL_RENDER_API_VTABLE_VULKAN_H
#define AL_RENDER_API_VTABLE_VULKAN_H

namespace al
{
    struct RenderApiVtable;

    void vulkan_render_api_vtable_fill(RenderApiVtable* table);
}

#endif
