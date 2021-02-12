
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
        , gbuffer{ 0 }
        , gpassShader{ 0 }
        , drawFramebufferToScreenShader{ 0 }
        , screenRectangleVb{ 0 }
        , screenRectangleIb{ 0 }
        , screenRectangleVa{ 0 }
        , indexBuffers{ }
        , vertexBuffers{ }
        , vertexArrays{ }
        , shaders{ }
    { }

    std::thread* Renderer::get_render_thread() noexcept
    {
        return &renderThread;
    }

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

    [[nodiscard]] GeometryCommandData* Renderer::add_geometry_command(GeometryCommandKey key) noexcept
    {
        GeometryCommandData* result = geometryCommandBuffer.get_previous().add_command(key);
        al_assert(result);
        return result;
    }

    IndexBufferHandle Renderer::create_index_buffer(uint32_t* indices, std::size_t count) noexcept
    {
        IndexBuffer** ib = indexBuffers.get();
        al_assert(ib);
        *ib = ::al::engine::create_index_buffer<EngineConfig::DEFAULT_RENDERER_TYPE>(indices, count);
        IndexBufferHandle handle
        {
            .isValid = true,
            .index = static_cast<uint64_t>(indexBuffers.get_direct_index(ib))
        };
        return handle;
    }

    inline IndexBuffer* Renderer::index_buffer(IndexBufferHandle handle) noexcept
    {
        return *indexBuffers.direct_accsess(static_cast<std::size_t>(handle.index));
    }

    VertexBufferHandle Renderer::create_vertex_buffer(const void* data, std::size_t size) noexcept
    {
        VertexBuffer** vb = vertexBuffers.get();
        al_assert(vb);
        *vb = ::al::engine::create_vertex_buffer<EngineConfig::DEFAULT_RENDERER_TYPE>(data, size);
        VertexBufferHandle handle
        {
            .isValid = true,
            .index = static_cast<uint64_t>(vertexBuffers.get_direct_index(vb))
        };
        return handle;
    }

    inline VertexBuffer* Renderer::vertex_buffer(VertexBufferHandle handle) noexcept
    {
        return *vertexBuffers.direct_accsess(static_cast<std::size_t>(handle.index));
    }

    VertexArrayHandle Renderer::create_vertex_array() noexcept
    {
        VertexArray** va = vertexArrays.get();
        al_assert(va);
        *va = ::al::engine::create_vertex_array<EngineConfig::DEFAULT_RENDERER_TYPE>();
        VertexArrayHandle handle
        {
            .isValid = true,
            .index = static_cast<uint64_t>(vertexArrays.get_direct_index(va))
        };
        return handle;
    }

    inline VertexArray* Renderer::vertex_array(VertexArrayHandle handle) noexcept
    {
        return *vertexArrays.direct_accsess(static_cast<std::size_t>(handle.index));
    }

    ShaderHandle Renderer::create_shader(std::string_view vertexShaderSrc, std::string_view fragmentShaderSrc) noexcept
    {
        Shader** s = shaders.get();
        al_assert(s);
        *s = ::al::engine::create_shader<EngineConfig::DEFAULT_RENDERER_TYPE>(vertexShaderSrc, fragmentShaderSrc);
        ShaderHandle handle
        {
            .isValid = true,
            .index = static_cast<uint64_t>(shaders.get_direct_index(s))
        };
        return handle;
    }

    inline Shader* Renderer::shader(ShaderHandle handle) noexcept
    {
        return *shaders.direct_accsess(static_cast<std::size_t>(handle.index));
    }

    FramebufferHandle Renderer::create_framebuffer(const FramebufferDescription& description) noexcept
    {
        Framebuffer** f = framebuffers.get();
        al_assert(f);
        *f = ::al::engine::create_framebuffer<EngineConfig::DEFAULT_RENDERER_TYPE>(description);
        FramebufferHandle handle
        {
            .isValid = true,
            .index = static_cast<uint64_t>(framebuffers.get_direct_index(f))
        };
        return handle;
    }

    inline Framebuffer* Renderer::framebuffer(FramebufferHandle handle) noexcept
    {
        return *framebuffers.direct_accsess(static_cast<std::size_t>(handle.index));
    }

    void Renderer::render_update() noexcept
    {
        initialize_renderer();
        {
            al_profile_scope("Renderer post-init");
            {
                FramebufferDescription gbufferDesciption;
                gbufferDesciption.attachments =
                {
                    FramebufferAttachmentType::RGB_8,               // Position
                    FramebufferAttachmentType::RGB_8,               // Normal
                    FramebufferAttachmentType::RGB_8,               // Albedo
                    FramebufferAttachmentType::DEPTH_24_STENCIL_8   // Depth + stencil (maybe?)
                };
                gbufferDesciption.width = window->get_params()->width;
                gbufferDesciption.height = window->get_params()->height;
                gbuffer = create_framebuffer(gbufferDesciption);
            }
            {
                FileHandle* vertGpassShaderSrc = FileSystem::sync_load(EngineConfig::DEFFERED_GEOMETRY_PASS_VERT_SHADER_PATH, FileLoadMode::READ);
                FileHandle* fragGpassShaderSrc = FileSystem::sync_load(EngineConfig::DEFFERED_GEOMETRY_PASS_FRAG_SHADER_PATH, FileLoadMode::READ);
                const char* vertGpassShaderStr = reinterpret_cast<const char*>(vertGpassShaderSrc->memory);
                const char* fragGpassShaderStr = reinterpret_cast<const char*>(fragGpassShaderSrc->memory);
                gpassShader = create_shader(vertGpassShaderStr, fragGpassShaderStr);
                FileSystem::free_handle(vertGpassShaderSrc);
                FileSystem::free_handle(fragGpassShaderSrc);
                shader(gpassShader)->bind();
                shader(gpassShader)->set_int(EngineConfig::DEFFERED_GEOMETRY_PASS_DIFFUSE_TEXTURE_NAME, EngineConfig::DEFFERED_GEOMETRY_PASS_DIFFUSE_TEXTURE_LOCATION);
            }
            {
                FileHandle* vertDrawFramebufferToScreenShaderSrc = FileSystem::sync_load(EngineConfig::DRAW_FRAMEBUFFER_TO_SCREEN_VERT_SHADER_PATH, FileLoadMode::READ);
                FileHandle* fragDrawFramebufferToScreenShaderSrc = FileSystem::sync_load(EngineConfig::DRAW_FRAMEBUFFER_TO_SCREEN_FRAG_SHADER_PATH, FileLoadMode::READ);
                const char* vertDrawFramebufferToScreenShaderStr = reinterpret_cast<const char*>(vertDrawFramebufferToScreenShaderSrc->memory);
                const char* fragDrawFramebufferToScreenShaderStr = reinterpret_cast<const char*>(fragDrawFramebufferToScreenShaderSrc->memory);
                drawFramebufferToScreenShader = create_shader(vertDrawFramebufferToScreenShaderStr, fragDrawFramebufferToScreenShaderStr);
                FileSystem::free_handle(vertDrawFramebufferToScreenShaderSrc);
                FileSystem::free_handle(fragDrawFramebufferToScreenShaderSrc);
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
                screenRectangleVb = create_vertex_buffer(screenPlaneVertices, sizeof(screenPlaneVertices));
                vertex_buffer(screenRectangleVb)->set_layout(BufferLayout::ElementContainer{
                    BufferElement{ ShaderDataType::Float2, false }, // Position
                    BufferElement{ ShaderDataType::Float2, false }  // Uv
                });
                screenRectangleIb = create_index_buffer(screenPlaneIndices, sizeof(screenPlaneIndices) / sizeof(uint32_t));
                screenRectangleVa = create_vertex_array();
                vertex_array(screenRectangleVa)->set_vertex_buffer(vertex_buffer(screenRectangleVb));
                vertex_array(screenRectangleVa)->set_index_buffer(index_buffer(screenRectangleIb));
            }
            // Enable vsync
            set_vsync_state(true);
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
                    framebuffer(gbuffer)->bind();
                    shader(gpassShader)->bind();
                    clear_buffers();
                    // Set view-projection matrix
                    const float4x4 vpMatrix = (renderCamera->get_projection() * renderCamera->get_view()).transposed();
                    shader(gpassShader)->set_mat4(EngineConfig::SHADER_VIEW_PROJECTION_MATRIX_UNIFORM_NAME, vpMatrix);
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
                        if (!data->diffuseTexture)
                        {
                            al_log_error(LOG_CATEGORY_RENDERER, "Trying to process draw command, but diffuse texture is null");
                            return;
                        }
                        data->diffuseTexture->bind(EngineConfig::DEFFERED_GEOMETRY_PASS_DIFFUSE_TEXTURE_LOCATION);
                        shader(gpassShader)->set_mat4(EngineConfig::SHADER_MODEL_MATRIX_UNIFORM_NAME, data->trf.matrix.transposed());
                        draw(data->va);
                    });
                    current.clear();
                }
                {
                    al_profile_scope("Draw to screen pass");
                    // Bind buffer and shader
                    bind_screen_framebuffer();
                    shader(drawFramebufferToScreenShader)->bind();
                    clear_buffers(); // Probably this is not needed
                    // Attach texture to be drawn on screen
                    framebuffer(gbuffer)->bind_attachment_to_slot(2, EngineConfig::SCREEN_PASS_SOURCE_BUFFER_TEXTURE_LOCATION);
                    shader(drawFramebufferToScreenShader)->set_int(EngineConfig::SCREEN_PASS_SOURCE_BUFFER_TEXTURE_NAME, EngineConfig::SCREEN_PASS_SOURCE_BUFFER_TEXTURE_LOCATION);
                    // Draw screen quad with selected texture
                    set_depth_test_state(false);
                    draw(vertex_array(screenRectangleVa));
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
            destroy_framebuffer<EngineConfig::DEFAULT_RENDERER_TYPE>(framebuffer(gbuffer));
            destroy_shader<EngineConfig::DEFAULT_RENDERER_TYPE>(shader(gpassShader));
            destroy_shader<EngineConfig::DEFAULT_RENDERER_TYPE>(shader(drawFramebufferToScreenShader));
            destroy_vertex_array<EngineConfig::DEFAULT_RENDERER_TYPE>(vertex_array(screenRectangleVa));
            destroy_vertex_buffer<EngineConfig::DEFAULT_RENDERER_TYPE>(vertex_buffer(screenRectangleVb));
            destroy_index_buffer<EngineConfig::DEFAULT_RENDERER_TYPE>(index_buffer(screenRectangleIb));
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
