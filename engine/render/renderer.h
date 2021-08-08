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

        RenderProgram* vs;
        RenderProgram* fs;
        RenderPass* renderPass;
        Array<Texture*> swapChainTextures;
        Array<Framebuffer*> swapChainFramebuffers;
        // FramebufferDescription* fbDescription;
        // RenderStage* stage;
        // Array<Framebuffer*> stageFramebuffers;

        RenderCommandBuffer* commandBuffer;
    };

    void renderer_construct      (Renderer* renderer, RendererInitData* initData);
    void renderer_destroy        (Renderer* renderer);
    void renderer_render         (Renderer* renderer);
    void renderer_handle_resize  (Renderer* renderer);
}

#endif
