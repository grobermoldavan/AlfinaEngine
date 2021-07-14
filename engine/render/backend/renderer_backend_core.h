#ifndef AL_RENDERER_BACKEND_CORE_H
#define AL_RENDERER_BACKEND_CORE_H

#include "engine/memory/memory.h"
#include "engine/platform/platform.h"
#include "engine/math/math.h"

#include "renderer_backend_types.h"
#include "texture_formats.h"
#include "texture_base.h"
#include "framebuffer_base.h"
#include "render_stage_base.h"

namespace al
{
    struct RenderVertex
    {
        f32_3 position;
        f32_3 normal;
        f32_2 uv;
    };

    struct RendererBackendCreateInfo
    {
        enum Flags : u64
        {
            IS_DEBUG = 1 << 0,
        };
        AllocatorBindings bindings;
        const char* applicationName;
        PlatformWindow* window;
        u64 flags;
    };

    struct RendererBackend { /* Implemented at a backend level */ };

    struct RendererBackendVtable
    {
        RendererBackend* (*create)(RendererBackendCreateInfo* createInfo);
        void (*destroy)(RendererBackend* backend);
        void (*handle_resize)(RendererBackend* backend);
        void (*begin_frame)(RendererBackend* backend);
        void (*end_frame)(RendererBackend* backend);
        PointerWithSize<Texture*> (*get_swap_chain_textures)(RendererBackend* backend);
        uSize (*get_active_swap_chain_texture_index)(RendererBackend* backend);
        TextureVtable texture;
        FramebufferVtable framebuffer;
        ShaderProgramVtable shaderProgram;
        RenderStageVtable renderStage;
    };
}

#endif
