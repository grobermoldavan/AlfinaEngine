
#include "user_application.h"

void construct(UserApplicationSubsystem* subsystem, UserApplication* application)
{
    al_log_message("Application subsystem construct");
    subsystem->frameCounter = 0;
}

void destroy(UserApplicationSubsystem* subsystem, UserApplication* application)
{
    al_log_message("Application subsystem destroy");
}

void update(UserApplicationSubsystem* subsystem, UserApplication* application)
{
    al_log_message("Application subsystem update %lld", subsystem->frameCounter++);
}

void resize(UserApplicationSubsystem* subsystem, UserApplication* application)
{
    al_log_message("Application subsystem resize");
}

void construct(UserRenderSubsystem* subsystem, UserApplication* application)
{
    al_log_message("Render subsystem construct");

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
        subsystem->vs = renderer->vt.program_create(&programCreateInfo);
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
        subsystem->fs = renderer->vt.program_create(&programCreateInfo);
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
        subsystem->renderPass = renderer->vt.render_pass_create(&renderPassCreateInfo);
    }
    {
        GraphicsRenderPipelineCreateInfo pipelineCreateInfo
        {
            .device                 = renderer->device,
            .pass                   = subsystem->renderPass,
            .vertexProgram          = subsystem->vs,
            .fragmentProgram        = subsystem->fs,
            .frontStencilOpState    = nullptr,
            .backStencilOpState     = nullptr,
            .depthTestState         = nullptr,
            .subpassIndex           = 0,
            .poligonMode            = PipelinePoligonMode::FILL,
            .cullMode               = PipelineCullMode::BACK,
            .frontFace              = PipelineFrontFace::CLOCKWISE,
            .multisamplingType      = MultisamplingType::SAMPLE_1,
        };
        subsystem->renderPipeline = renderer->vt.render_pipeline_graphics_create(&pipelineCreateInfo);
    }
    {
        uSize numSwapChainTextures = renderer->vt.get_swap_chain_textures_num(renderer->device);
        array_construct(&subsystem->swapChainTextures, &persistentAllocator, numSwapChainTextures);
        for (al_iterator(it, subsystem->swapChainTextures))
        {
            *get(it) = renderer->vt.get_swap_chain_texture(renderer->device, to_index(it));
        }
    }
    {
        array_construct(&subsystem->swapChainFramebuffers, &persistentAllocator, subsystem->swapChainTextures.size);
        for (al_iterator(it, subsystem->swapChainFramebuffers))
        {
            Texture* attachments[] = { subsystem->swapChainTextures[to_index(it)] };
            FramebufferCreateInfo framebufferCreateInfo
            {
                .attachments    = { attachments, array_size(attachments) },
                .pass           = subsystem->renderPass,
                .device         = renderer->device,
            };
            *get(it) = renderer->vt.framebuffer_create(&framebufferCreateInfo);
        }
    }
}

void destroy(UserRenderSubsystem* subsystem, UserApplication* application)
{
    al_log_message("Render subsystem destroy");

    using namespace al;
    Renderer* renderer = &application->renderer;

    renderer->vt.device_wait(renderer->device);
    for (al_iterator(it, subsystem->swapChainFramebuffers)) { renderer->vt.framebuffer_destroy(*get(it)); }
    for (al_iterator(it, subsystem->swapChainTextures)) { renderer->vt.texture_destroy(*get(it)); }
    array_destruct(&subsystem->swapChainFramebuffers);
    array_destruct(&subsystem->swapChainTextures);
    renderer->vt.render_pipeline_destroy(subsystem->renderPipeline);
    renderer->vt.render_pass_destroy(subsystem->renderPass);
    renderer->vt.program_destroy(subsystem->vs);
    renderer->vt.program_destroy(subsystem->fs);
}

void update(UserRenderSubsystem* subsystem, UserApplication* application)
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
            .pipeline = subsystem->renderPipeline,
            .framebuffer = subsystem->swapChainFramebuffers[swapChainIndex],
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

void resize(UserRenderSubsystem* subsystem, UserApplication* application)
{
    al_log_message("Render subsystem resize");
}

int main(int argc, char* argv[])
{
    UserApplication userApplication = { };
    al::application_run(&userApplication, { argc, argv });
}
