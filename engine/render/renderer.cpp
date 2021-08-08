
#include "renderer.h"

namespace al
{
    void renderer_construct(Renderer* renderer, RendererInitData* initData)
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
        {
            PlatformFile vertexShader = platform_file_load(&initData->frameAllocator, platform_path("assets", "shaders", "simple.vert.spv"), PlatformFileLoadMode::READ);
            defer(platform_file_unload(&initData->frameAllocator, vertexShader));
            RenderProgramCreateInfo programCreateInfo
            {
                .device = renderer->device,
                .bytecode = (u32*)vertexShader.memory,
                .codeSizeBytes = vertexShader.sizeBytes,
            };
            renderer->vs = renderer->vt.program_create(&programCreateInfo);
        }
        {
            PlatformFile fragmentShader = platform_file_load(&initData->frameAllocator, platform_path("assets", "shaders", "simple.frag.spv"), PlatformFileLoadMode::READ);
            defer(platform_file_unload(&initData->frameAllocator, fragmentShader));
            RenderProgramCreateInfo programCreateInfo
            {
                .device = renderer->device,
                .bytecode = (u32*)fragmentShader.memory,
                .codeSizeBytes = fragmentShader.sizeBytes,
            };
            renderer->fs = renderer->vt.program_create(&programCreateInfo);
        }
        {
            RenderPassCreateInfo::Subpass subpasses[] =
            {
                {
                    .colorRefs      = u32(1) << 0,
                    .inputRefs      = 0,
                    .resolveRefs    = 0,
                    .depthOp        = RenderPassCreateInfo::DepthOp::NOTHING, //RenderPassCreateInfo::DepthOp::READ_WRITE,
                },
                // {
                //     .colorRefs      = u32(1) << 1,
                //     .inputRefs      = u32(1) << 0,
                //     .resolveRefs    = 0,
                //     .depthOp        = RenderPassCreateInfo::DepthOp::NOTHING,
                // },
            };
            RenderPassCreateInfo::Attachment colorAttachments[] =
            {
                // { TextureFormat::RGBA_8, AttachmentLoadOp::CLEAR, AttachmentStoreOp::STORE, },
                { TextureFormat::SWAP_CHAIN, AttachmentLoadOp::CLEAR, AttachmentStoreOp::STORE, },
            };
            RenderPassCreateInfo::Attachment depthStencilAttachment
            {
                .format = TextureFormat::DEPTH_STENCIL,
                .loadOp = AttachmentLoadOp::CLEAR,
                .storeOp = AttachmentStoreOp::STORE,
            };
            RenderPassCreateInfo renderPassCreateInfo
            {
                .subpasses              = { subpasses, array_size(subpasses) },
                .colorAttachments       = { colorAttachments, array_size(colorAttachments) },
                .depthStencilAttachment = nullptr, //&depthStencilAttachment,
                .device                 = renderer->device,
            };
            renderer->renderPass = renderer->vt.render_pass_create(&renderPassCreateInfo);
        }
        {
            uSize numSwapChainTextures = renderer->vt.get_swap_chain_textures_num(renderer->device);
            array_construct(&renderer->swapChainTextures, &initData->persistentAllocator, numSwapChainTextures);
            for (al_iterator(it, renderer->swapChainTextures))
            {
                *get(it) = renderer->vt.get_swap_chain_texture(renderer->device, to_index(it));
            }
        }
        {
            array_construct(&renderer->swapChainFramebuffers, &initData->persistentAllocator, renderer->swapChainTextures.size);
            for (al_iterator(it, renderer->swapChainFramebuffers))
            {
                Texture* attachments[] = { renderer->swapChainTextures[to_index(it)] };
                FramebufferCreateInfo framebufferCreateInfo
                {
                    .attachments    = { attachments, array_size(attachments) },
                    .pass           = renderer->renderPass,
                    .device         = renderer->device,
                };
                *get(it) = renderer->vt.framebuffer_create(&framebufferCreateInfo);
            }
        }
        {
            
        }
    }

    void renderer_destruct(Renderer* renderer)
    {
        for (al_iterator(it, renderer->swapChainFramebuffers)) { renderer->vt.framebuffer_destroy(*get(it)); }
        for (al_iterator(it, renderer->swapChainTextures)) { renderer->vt.texture_destroy(*get(it)); }
        array_destruct(&renderer->swapChainFramebuffers);
        array_destruct(&renderer->swapChainTextures);
        renderer->vt.render_pass_destroy(renderer->renderPass);
        renderer->vt.program_destroy(renderer->vs);
        renderer->vt.program_destroy(renderer->fs);
        renderer->vt.device_destroy(renderer->device);
    }

    void renderer_render(Renderer* renderer)
    {
    }

    void renderer_handle_resize(Renderer* renderer)
    {
    }
}

#include "render_api_abstraction_layer/render_api_abstraction_layer.cpp"
