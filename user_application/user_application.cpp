
#include "user_application.h"

#include <cstdio>

void user_create(UserApplication* application, al::CommandLineArgs args)
{
    std::fprintf(stdout, "User create function\n");
}

void user_destroy(UserApplication* application)
{
    std::fprintf(stdout, "User destroy function\n");
}

void user_update(UserApplication* application)
{
    
}

void user_handle_window_resize(UserApplication* application)
{
    al::u32 width = al::platform_window_get_current_width(&application->window);
    al::u32 height = al::platform_window_get_current_height(&application->window);
    std::fprintf(stdout, "User window resized function : %d %d\n", width, height);
}

void user_renderer_construct(UserApplication* application)
{
    using namespace al;
    Renderer* renderer = &application->renderer;

    AllocatorBindings frameAllocator = get_allocator_bindings(&application->frameAllocator);
    AllocatorBindings persistentAllocator = get_allocator_bindings(&application->pool);
    {
        PlatformFile vertexShader;
        unwrap(platform_file_load(&vertexShader, platform_path("assets", "shaders", "simple.vert.spv"), PlatformFileMode::READ));
        PlatformFileContent shaderBytecode = platform_file_read(&vertexShader, &frameAllocator);
        defer
        (
            platform_file_free_content(&shaderBytecode);
            platform_file_unload(&vertexShader);
        );
        RenderProgramCreateInfo programCreateInfo
        {
            .device = renderer->device,
            .bytecode = (u32*)shaderBytecode.memory,
            .codeSizeBytes = shaderBytecode.sizeBytes,
        };
        application->vs = renderer->vt.program_create(&programCreateInfo);
    }
    {
        PlatformFile fragmentShader;
        unwrap(platform_file_load(&fragmentShader, platform_path("assets", "shaders", "simple.frag.spv"), PlatformFileMode::READ));
        PlatformFileContent shaderBytecode = platform_file_read(&fragmentShader, &frameAllocator);
        defer
        (
            platform_file_free_content(&shaderBytecode);
            platform_file_unload(&fragmentShader);
        );
        RenderProgramCreateInfo programCreateInfo
        {
            .device = renderer->device,
            .bytecode = (u32*)shaderBytecode.memory,
            .codeSizeBytes = shaderBytecode.sizeBytes,
        };
        application->fs = renderer->vt.program_create(&programCreateInfo);
    }
    {
        u32 subpassColorRefs[] = { 0 };
        RenderPassCreateInfo::Subpass subpasses[] =
        {
            {
                .colorRefs      = { subpassColorRefs, array_size(subpassColorRefs) },
                .inputRefs      = { nullptr, 0 },
                .resolveRefs    = { nullptr, 0 },
                .depthOp        = RenderPassCreateInfo::DepthOp::NOTHING,
            },
        };
        RenderPassCreateInfo::Attachment colorAttachments[] =
        {
            { TextureFormat::SWAP_CHAIN, AttachmentLoadOp::CLEAR, AttachmentStoreOp::STORE, MultisamplingType::SAMPLE_1 },
        };
        RenderPassCreateInfo renderPassCreateInfo
        {
            .subpasses              = { subpasses, array_size(subpasses) },
            .colorAttachments       = { colorAttachments, array_size(colorAttachments) },
            .depthStencilAttachment = nullptr,
            .device                 = renderer->device,
        };
        application->renderPass = renderer->vt.render_pass_create(&renderPassCreateInfo);
    }
    {
        GraphicsRenderPipelineCreateInfo pipelineCreateInfo
        {
            .device                 = renderer->device,
            .pass                   = application->renderPass,
            .vertexProgram          = application->vs,
            .fragmentProgram        = application->fs,
            .frontStencilOpState    = nullptr,
            .backStencilOpState     = nullptr,
            .depthTestState         = nullptr,
            .subpassIndex           = 0,
            .poligonMode            = PipelinePoligonMode::FILL,
            .cullMode               = PipelineCullMode::BACK,
            .frontFace              = PipelineFrontFace::CLOCKWISE,
            .multisamplingType      = MultisamplingType::SAMPLE_1,
        };
        application->renderPipeline = renderer->vt.render_pipeline_graphics_create(&pipelineCreateInfo);
    }
    {
        uSize numSwapChainTextures = renderer->vt.get_swap_chain_textures_num(renderer->device);
        array_construct(&application->swapChainTextures, &persistentAllocator, numSwapChainTextures);
        for (al_iterator(it, application->swapChainTextures))
        {
            *get(it) = renderer->vt.get_swap_chain_texture(renderer->device, to_index(it));
        }
    }
    {
        array_construct(&application->swapChainFramebuffers, &persistentAllocator, application->swapChainTextures.size);
        for (al_iterator(it, application->swapChainFramebuffers))
        {
            Texture* attachments[] = { application->swapChainTextures[to_index(it)] };
            FramebufferCreateInfo framebufferCreateInfo
            {
                .attachments    = { attachments, array_size(attachments) },
                .pass           = application->renderPass,
                .device         = renderer->device,
            };
            *get(it) = renderer->vt.framebuffer_create(&framebufferCreateInfo);
        }
    }
}

void user_renderer_destroy(UserApplication* application)
{
    using namespace al;
    Renderer* renderer = &application->renderer;

    renderer->vt.device_wait(renderer->device);
    for (al_iterator(it, application->swapChainFramebuffers)) { renderer->vt.framebuffer_destroy(*get(it)); }
    for (al_iterator(it, application->swapChainTextures)) { renderer->vt.texture_destroy(*get(it)); }
    array_destruct(&application->swapChainFramebuffers);
    array_destruct(&application->swapChainTextures);
    renderer->vt.render_pipeline_destroy(application->renderPipeline);
    renderer->vt.render_pass_destroy(application->renderPass);
    renderer->vt.program_destroy(application->vs);
    renderer->vt.program_destroy(application->fs);
}

void user_renderer_render(UserApplication* application)
{
    using namespace al;
    Renderer* renderer = &application->renderer;

    renderer->vt.begin_frame(renderer->device);
    {
        CommandBufferRequestInfo commandBufferRequest { renderer->device, CommandBufferUsage::GRAPHICS };
        CommandBuffer* commandBuffer = renderer->vt.command_buffer_request(&commandBufferRequest);
        const uSize swapChainIndex = renderer->vt.get_active_swap_chain_texture_index(renderer->device);
        CommandBindPipelineInfo bindInfo
        {
            .pipeline = application->renderPipeline,
            .framebuffer = application->swapChainFramebuffers[swapChainIndex],
        };
        renderer->vt.command_bind_pipeline(commandBuffer, &bindInfo);
        CommandDrawInfo drawInfo
        {
            .vertexCount = 3,
        };
        renderer->vt.command_draw(commandBuffer, &drawInfo);
        renderer->vt.command_buffer_submit(commandBuffer);
    }
    renderer->vt.end_frame(renderer->device);
}

int main(int argc, char* argv[])
{
    UserApplication userApplication = { };
    al::application_run(&userApplication, { argc, argv });
}
