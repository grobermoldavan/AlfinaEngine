#ifndef AL_RENDERER_H
#define AL_RENDERER_H

#include "engine/memory/memory.h"
#include "engine/utilities/utilities.h"
#include "backend/renderer_backend_types.h"
#include "backend/renderer_backend_core.h"
#include "backend/vulkan/renderer_backend_vulkan.h"

namespace al
{
    struct RendererInitData
    {
        AllocatorBindings bindings;
        PlatformWindow* window;
        RendererBackendType backendType;
    };

    struct Renderer
    {
        RendererBackendVtable vt;
        RendererBackend* backend;

        ShaderProgram* vertexShader;
        ShaderProgram* fragmentShader;
        Array<Framebuffer*> framebuffers;
        RenderStage* stage;
    };

    void renderer_construct      (Renderer* renderer, RendererInitData* initData);
    void renderer_destroy        (Renderer* renderer);
    void renderer_render         (Renderer* renderer);
    void renderer_handle_resize  (Renderer* renderer);
}

#endif
