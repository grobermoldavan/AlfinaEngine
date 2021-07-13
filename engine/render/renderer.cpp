
#include "renderer.h"

namespace al
{
    /*
    @TODO : 
    1. finish creating nessesary rendering primitives (stage, framebuffer, textures and buffers)
    2. setup basic rendering process with a single colored window as a result
    3. need to create swap chain interface for renderer. We will need to iterate over each swap chain image on
       a renderer level. This is needed so we can create framebuffers for each swap chain image.
    4. after we created all render stages (one in the simplest case) and all framebuffers (1 * numberOfSwapChainImages
       in the simplest case) we need to bind them each frame and process binded render stage (draw stuff to the framebuffer)
    5. Cleanup code
    6. Finish stencil testing support (renderer GPU interface is required so we can check if stencil is supported on a user's machine)
       (this can look like this : bool isSupported = renderer->gpu.is_stencil_supported(renderer->backend);)
    */
    void renderer_construct(Renderer* renderer, RendererInitData* initData)
    {
        RendererBackendCreateInfo createInfo
        {
            .bindings           = initData->bindings,
            .applicationName    = "Application Name",
            .window             = initData->window,
            .flags              = RendererBackendCreateInfo::IS_DEBUG,
        };
        switch (initData->backendType)
        {
            case RendererBackendType::VULKAN: vulkan_backend_fill_vtable(&renderer->vt); break;
        }
        renderer->backend = renderer->vt.create(&createInfo);
        {
            PlatformFile vertexShader = platform_file_load(&initData->bindings, platform_path("assets", "shaders", "simple.vert.spv"), PlatformFileLoadMode::READ);
            PlatformFile fragmentShader = platform_file_load(&initData->bindings, platform_path("assets", "shaders", "simple.frag.spv"), PlatformFileLoadMode::READ);
            defer(platform_file_unload(&initData->bindings, vertexShader));
            defer(platform_file_unload(&initData->bindings, fragmentShader));
            renderer->vertexShader = renderer->vt.shaderProgram.create(renderer->backend, vertexShader);
            renderer->fragmentShader = renderer->vt.shaderProgram.create(renderer->backend, fragmentShader);
            FramebufferDescription::AttachmentDescription framebufferAttachmentDescriptions[] =
            {
                {
                    .format = { /*whatever*/ },
                    .flags = FramebufferDescription::AttachmentDescription::SWAP_CHAIN_ATTACHMENT,
                },
                // {
                //     .format = TextureFormat::DEPTH_STENCIL,
                //     .flags = 0,
                // },
            };
            FramebufferDescription framebufferDescription
            {
                .attachmentDescriptions = { .ptr = framebufferAttachmentDescriptions, .size = array_size(framebufferAttachmentDescriptions), },
            };
            RenderStageCreateInfo stageCreateInfo
            {
                .type = RenderStageCreateInfo::GRAPHICS,
                .graphics =
                {
                    .stencilOpState =
                    {
                        // .compareMask    = 0xFF,
                        // .writeMask      = 0xFF,
                        // .reference      = 1,
                        // .failOp         = StencilOp::KEEP,
                        // .passOp         = StencilOp::REPLACE,
                        // .depthFailOp    = StencilOp::KEEP,
                        // .compareOp      = CompareOp::ALWAYS,
                    },
                    .vertexShader               = renderer->vertexShader,
                    .fragmentShader             = renderer->fragmentShader,
                    .framebufferDescription     = &framebufferDescription,
                    .hasDepthStencilAttachment  = true,
                },
            };
            renderer->stage = renderer->vt.renderStage.create(renderer->backend, &stageCreateInfo);
        }
    }

    void renderer_destruct(Renderer* renderer)
    {
        renderer->vt.renderStage.destroy(renderer->backend, renderer->stage);
        renderer->vt.shaderProgram.destroy(renderer->backend, renderer->vertexShader);
        renderer->vt.shaderProgram.destroy(renderer->backend, renderer->fragmentShader);
        renderer->vt.destroy(renderer->backend);
    }

    void renderer_render(Renderer* renderer)
    {
        // renderer->vt.begin_frame(renderer->backend);
        // renderer->vt.end_frame(renderer->backend);
    }

    void renderer_handle_resize(Renderer* renderer)
    {
        renderer->vt.handle_resize(renderer->backend);
    }
}

#include "backend/vulkan/renderer_backend_vulkan.cpp"
