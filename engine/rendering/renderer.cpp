
#include "renderer.h"

#include "engine/file_system/file_system.h"

#include "utilities/safe_cast.h"

namespace al::engine
{
    Renderer::Renderer(OsWindow* window) noexcept
        : renderThread{ &Renderer::render_update, this }
        , shouldRun{ true }
        , window{ window }
        , onFrameProcessStart{ create_thread_event() }
        , onFrameProcessEnd{ create_thread_event() }
        , onCommandBufferToggled{ create_thread_event() }
        , gbuffer{ nullptr }
        , gpassShader{ nullptr }
        , drawFramebufferToScreenShader{ nullptr }
        , screenRectangleVb{ nullptr }
        , screenRectangleIb{ nullptr }
        , screenRectangleVa{ nullptr }
    { }

    void Renderer::terminate() noexcept
    {
        // @NOTE :  Can't join render in the destructor because on joining thread will call
        //          virtual terminate_renderer method, but the child will be already terminated
        shouldRun = false;
        start_process_frame();
        al_exception_wrap(renderThread.join());
    }
        
    void Renderer::start_process_frame() noexcept
    {
        al_assert(!onFrameProcessStart->is_invoked());
        onFrameProcessEnd->reset();
        onFrameProcessStart->invoke();
    }

    void Renderer::wait_for_render_finish() noexcept
    {
        al_profile_function();

        const bool waitResult = onFrameProcessEnd->wait_for(std::chrono::seconds{ 1 });
        al_assert(waitResult); // Event was not fired. Probably something happend to render thread
    }

    void Renderer::wait_for_command_buffers_toggled() noexcept
    {
        al_profile_function();

        const bool waitResult = onCommandBufferToggled->wait_for(std::chrono::seconds{ 1 });
        al_assert(waitResult); // Event was not fired. Probably something happend to render thread
        onCommandBufferToggled->reset();
    }

    void Renderer::set_camera(const RenderCamera* camera) noexcept
    {
        renderCamera = camera;
    }

    void Renderer::add_render_command(const RenderCommand& command) noexcept
    {
        RenderCommand* result = renderCommandBuffer.get_previous().push(command);
        al_assert(result);
    }

    [[nodiscard]] GeometryCommandData* Renderer::add_geometry(GeometryCommandKey key) noexcept
    {
        GeometryCommandData* result = geometryCommandBuffer.get_previous().add_command(key);
        al_assert(result);
        return result;
    }

    void Renderer::render_update() noexcept
    {
        initialize_renderer();
        {
            al_profile_scope("Renderer post-init");
            FileSystem* fileSystem = FileSystem::get();
            {
                FramebufferDescription gbufferDesciption;
                gbufferDesciption.attachments =
                {
                    FramebufferAttachmentType::RGB_8,               // Position
                    FramebufferAttachmentType::RGB_8,               // Normal
                    FramebufferAttachmentType::RGB_8,               // Albedo
                    FramebufferAttachmentType::DEPTH_24_STENCIL_8   // Depth + stencil (maybe?)
                };
                gbufferDesciption.width = safe_cast_uint64_to_uint32(window->get_params()->width);
                gbufferDesciption.height = safe_cast_uint64_to_uint32(window->get_params()->height);
                gbuffer = create_framebuffer<EngineConfig::DEFAULT_RENDERER_TYPE>(gbufferDesciption);
            }
            {
                FileHandle* vertGpassShaderSrc = fileSystem->sync_load(EngineConfig::DEFFERED_GEOMETRY_PASS_VERT_SHADER_PATH, FileLoadMode::READ);
                FileHandle* fragGpassShaderSrc = fileSystem->sync_load(EngineConfig::DEFFERED_GEOMETRY_PASS_FRAG_SHADER_PATH, FileLoadMode::READ);
                const char* vertGpassShaderStr = reinterpret_cast<const char*>(vertGpassShaderSrc->memory);
                const char* fragGpassShaderStr = reinterpret_cast<const char*>(fragGpassShaderSrc->memory);
                gpassShader = create_shader<EngineConfig::DEFAULT_RENDERER_TYPE>(vertGpassShaderStr, fragGpassShaderStr);
                fileSystem->free_handle(vertGpassShaderSrc);
                fileSystem->free_handle(fragGpassShaderSrc);

                gpassShader->bind();
                gpassShader->set_int(EngineConfig::DEFFERED_GEOMETRY_PASS_DIFFUSE_TEXTURE_NAME, EngineConfig::DEFFERED_GEOMETRY_PASS_DIFFUSE_TEXTURE_LOCATION);
            }
            {
                FileHandle* vertDrawFramebufferToScreenShaderSrc = fileSystem->sync_load(EngineConfig::DRAW_FRAMEBUFFER_TO_SCREEN_VERT_SHADER_PATH, FileLoadMode::READ);
                FileHandle* fragDrawFramebufferToScreenShaderSrc = fileSystem->sync_load(EngineConfig::DRAW_FRAMEBUFFER_TO_SCREEN_FRAG_SHADER_PATH, FileLoadMode::READ);
                const char* vertDrawFramebufferToScreenShaderStr = reinterpret_cast<const char*>(vertDrawFramebufferToScreenShaderSrc->memory);
                const char* fragDrawFramebufferToScreenShaderStr = reinterpret_cast<const char*>(fragDrawFramebufferToScreenShaderSrc->memory);
                drawFramebufferToScreenShader = create_shader<EngineConfig::DEFAULT_RENDERER_TYPE>(vertDrawFramebufferToScreenShaderStr, fragDrawFramebufferToScreenShaderStr);
                fileSystem->free_handle(vertDrawFramebufferToScreenShaderSrc);
                fileSystem->free_handle(fragDrawFramebufferToScreenShaderSrc);
            }
            {
                static float screenPlaneVertices[] =
                {
                    -1.0f, -1.0f, 0.0f, 0.0f,   // Bottom left
                     1.0f, -1.0f, 1.0f, 0.0f,   // Bottom right
                    -1.0f,  1.0f, 0.0f, 1.0f,   // Top left
                     1.0f,  1.0f, 1.0f, 1.0f    // Top right
                };
                static uint32_t screenPlaneIndices[] = 
                {
                    0, 1, 2,
                    2, 1, 3
                };
                screenRectangleVb = create_vertex_buffer<EngineConfig::DEFAULT_RENDERER_TYPE>(screenPlaneVertices, sizeof(screenPlaneVertices));
                screenRectangleVb->set_layout(BufferLayout::ElementContainer{
                    BufferElement{ ShaderDataType::Float2, false }, // Position
                    BufferElement{ ShaderDataType::Float2, false }  // Uv
                });
                screenRectangleIb = create_index_buffer<EngineConfig::DEFAULT_RENDERER_TYPE>(screenPlaneIndices, sizeof(screenPlaneIndices) / sizeof(uint32_t));
                screenRectangleVa = create_vertex_array<EngineConfig::DEFAULT_RENDERER_TYPE>();
                screenRectangleVa->set_vertex_buffer(screenRectangleVb);
                screenRectangleVa->set_index_buffer(screenRectangleIb);
            }
        }
        while (true)
        {
            wait_for_render_start();
            if (!shouldRun) { break; }
            {
                al_profile_scope("Render Frame");
                // @NOTE : Don't know where is the best place for buffers toggle
                {
                    al_profile_scope("Toggle command buffers");
                    renderCommandBuffer.toggle();
                    geometryCommandBuffer.toggle();
                    notify_command_buffers_toggled();
                }
                {
                    al_profile_scope("Process render commands");
                    RenderCommandBuffer& current = renderCommandBuffer.get_current();
                    current.for_each([](RenderCommand* command)
                    {
                        (*command)();
                    });
                    current.clear();
                }
                {
                    al_profile_scope("Geometry pass");
                    // Bind buffer and shader
                    gbuffer->bind();
                    gpassShader->bind();
                    clear_buffers();
                    // Set view-projection matrix
                    const float4x4 vpMatrix = (renderCamera->get_projection() * renderCamera->get_view()).transposed();
                    gpassShader->set_mat4(EngineConfig::SHADER_VIEW_PROJECTION_MATRIX_UNIFORM_NAME, vpMatrix);
                    // Draw geometry
                    GeometryCommandBuffer& current = geometryCommandBuffer.get_current();
                    current.sort();
                    current.for_each([this](GeometryCommandData* data)
                    {
                        if (!data->va)
                        {
                            al_log_error(LOG_CATEGORY_RENDERER, "Trying to process draw command, but vertex array is null");
                            return;
                        }
                        data->diffuseTexture->bind(EngineConfig::DEFFERED_GEOMETRY_PASS_DIFFUSE_TEXTURE_LOCATION);
                        gpassShader->set_mat4(EngineConfig::SHADER_MODEL_MATRIX_UNIFORM_NAME, data->trf.get_full_transform());
                        draw(data->va);
                    });
                    current.clear();
                }
                {
                    al_profile_scope("\"Draw to screen\" pass");
                    // Bind buffer and shader
                    bind_screen_framebuffer();
                    drawFramebufferToScreenShader->bind();
                    clear_buffers(); // Probably this is not needed
                    // Attach texture to be drawn on screen
                    gbuffer->bind_attachment_to_slot(0, EngineConfig::SCREEN_PASS_SOURCE_BUFFER_TEXTURE_LOCATION);
                    drawFramebufferToScreenShader->set_int(EngineConfig::SCREEN_PASS_SOURCE_BUFFER_TEXTURE_NAME, EngineConfig::SCREEN_PASS_SOURCE_BUFFER_TEXTURE_LOCATION);
                    // Draw screen quad with selected texture
                    set_depth_test_state(false);
                    draw(screenRectangleVa);
                    set_depth_test_state(true);
                }
                {
                    al_profile_scope("Swap render buffers");
                    swap_buffers();
                }
            }
            notify_render_finished();
        }
        {
            al_profile_scope("Renderer pre-terminate");

            destroy_framebuffer<EngineConfig::DEFAULT_RENDERER_TYPE>(gbuffer);
            destroy_shader<EngineConfig::DEFAULT_RENDERER_TYPE>(drawFramebufferToScreenShader);
            destroy_vertex_array<EngineConfig::DEFAULT_RENDERER_TYPE>(screenRectangleVa);
            destroy_vertex_buffer<EngineConfig::DEFAULT_RENDERER_TYPE>(screenRectangleVb);
            destroy_index_buffer<EngineConfig::DEFAULT_RENDERER_TYPE>(screenRectangleIb);
        }
        terminate_renderer();
    }

    void Renderer::wait_for_render_start() noexcept
    {
        al_profile_function();
        onFrameProcessStart->wait();
    }

    void Renderer::notify_render_finished() noexcept
    {
        al_assert(!onFrameProcessEnd->is_invoked());
        onFrameProcessStart->reset();
        onFrameProcessEnd->invoke();
    }

    void Renderer::notify_command_buffers_toggled() noexcept
    {
        al_assert(!onCommandBufferToggled->is_invoked());
        onCommandBufferToggled->invoke();
    }
}
