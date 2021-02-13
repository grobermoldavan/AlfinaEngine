
#include "renderer.h"

#include "engine/file_system/file_system.h"

#include "utilities/safe_cast.h"

namespace al::engine
{
    Renderer* Renderer::instance{ nullptr };

    void Renderer::allocate_space() noexcept
    {
        // @NOTE :  We (possibly in the future) will be able to switch renderer types (from OpenGL to Vulkan, for example)
        //          and this is will be done (probably) by recreating renderer instance of the new renderer type.
        //          To avoid allocating renderer memory in the pool over and over, we simply allocate space large enough to
        //          store any kind of renderer available for target system and reuse that space.
        instance = reinterpret_cast<Renderer*>(MemoryManager::get_stack()->allocate(internal::get_max_renderer_size_bytes()));
    }

    void Renderer::construct(RendererType type, OsWindow* window) noexcept
    {
        if (!instance)
        {
            return;
        }
        switch (type)
        {
            case RendererType::OPEN_GL: internal::placement_create_renderer<RendererType::OPEN_GL>(instance, window);
        }
    }

    void Renderer::destruct() noexcept
    {
        if (!instance)
        {
            return;
        }
        instance->terminate();
        instance->~Renderer();
    }

    Renderer* Renderer::get() noexcept
    {
        return instance;
    }

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
        , texture2ds{ }
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

    RendererIndexBufferHandle Renderer::create_index_buffer(uint32_t* indices, std::size_t count, IndexBufferCallback cb) noexcept
    {
        IndexBuffer** ib = indexBuffers.get();
        al_assert(ib);
        *ib = nullptr;
        RendererIndexBufferHandle handle
        {
            .isValid = true,
            .index = static_cast<uint64_t>(indexBuffers.get_direct_index(ib))
        };
        add_render_command([this, handle, indices, count, cb]()
        {
            index_buffer(handle) = ::al::engine::internal::create_index_buffer<EngineConfig::DEFAULT_RENDERER_TYPE>(indices, count);
            if (cb)
            {
                const_cast<IndexBufferCallback*>(&cb)->call(handle);
            }
        });
        return handle;
    }

    inline IndexBuffer*& Renderer::index_buffer(RendererIndexBufferHandle handle) noexcept
    {
        return *indexBuffers.direct_accsess(static_cast<std::size_t>(handle.index));
    }

    RendererVertexBufferHandle Renderer::create_vertex_buffer(const void* data, std::size_t size, VertexBufferCallback cb) noexcept
    {
        VertexBuffer** vb = vertexBuffers.get();
        al_assert(vb);
        *vb = nullptr;
        RendererVertexBufferHandle handle
        {
            .isValid = true,
            .index = static_cast<uint64_t>(vertexBuffers.get_direct_index(vb))
        };
        add_render_command([this, handle, data, size, cb]()
        {
            vertex_buffer(handle) = ::al::engine::internal::create_vertex_buffer<EngineConfig::DEFAULT_RENDERER_TYPE>(data, size);
            if (cb)
            {
                const_cast<VertexBufferCallback*>(&cb)->call(handle);
            }
        });
        return handle;
    }

    inline VertexBuffer*& Renderer::vertex_buffer(RendererVertexBufferHandle handle) noexcept
    {
        return *vertexBuffers.direct_accsess(static_cast<std::size_t>(handle.index));
    }

    RendererVertexArrayHandle Renderer::create_vertex_array(VertexArrayCallback cb) noexcept
    {
        VertexArray** va = vertexArrays.get();
        al_assert(va);
        *va = nullptr;
        RendererVertexArrayHandle handle
        {
            .isValid = true,
            .index = static_cast<uint64_t>(vertexArrays.get_direct_index(va))
        };
        add_render_command([this, handle, cb]()
        {
            vertex_array(handle) = ::al::engine::internal::create_vertex_array<EngineConfig::DEFAULT_RENDERER_TYPE>();
            if (cb)
            {
                const_cast<VertexArrayCallback*>(&cb)->call(handle);
            }
        });
        return handle;
    }

    inline VertexArray*& Renderer::vertex_array(RendererVertexArrayHandle handle) noexcept
    {
        return *vertexArrays.direct_accsess(static_cast<std::size_t>(handle.index));
    }

    RendererShaderHandle Renderer::create_shader(std::string_view vertexShaderSrc, std::string_view fragmentShaderSrc, ShaderCallback cb) noexcept
    {
        Shader** s = shaders.get();
        al_assert(s);
        *s = nullptr;
        RendererShaderHandle handle
        {
            .isValid = true,
            .index = static_cast<uint64_t>(shaders.get_direct_index(s))
        };
        add_render_command([this, handle, vertexShaderSrc, fragmentShaderSrc, cb]()
        {
            shader(handle) = ::al::engine::internal::create_shader<EngineConfig::DEFAULT_RENDERER_TYPE>(vertexShaderSrc, fragmentShaderSrc);
            if (cb)
            {
                const_cast<ShaderCallback*>(&cb)->call(handle);
            }
        });
        return handle;
    }

    inline Shader*& Renderer::shader(RendererShaderHandle handle) noexcept
    {
        return *shaders.direct_accsess(static_cast<std::size_t>(handle.index));
    }

    RendererFramebufferHandle Renderer::create_framebuffer(const FramebufferDescription& description, FramebufferCallback cb) noexcept
    {
        Framebuffer** f = framebuffers.get();
        al_assert(f);
        *f = nullptr;
        RendererFramebufferHandle handle
        {
            .isValid = true,
            .index = static_cast<uint64_t>(framebuffers.get_direct_index(f))
        };
        add_render_command([this, handle, description, cb]()
        {
            framebuffer(handle) = ::al::engine::internal::create_framebuffer<EngineConfig::DEFAULT_RENDERER_TYPE>(description);
            if (cb)
            {
                const_cast<FramebufferCallback*>(&cb)->call(handle);
            }
        });
        return handle;
    }

    inline Framebuffer*& Renderer::framebuffer(RendererFramebufferHandle handle) noexcept
    {
        return *framebuffers.direct_accsess(static_cast<std::size_t>(handle.index));
    }

    RendererTexture2dHandle Renderer::create_texture_2d(std::string_view path, Texture2dCallback cb) noexcept
    {
        Texture2d** tex = texture2ds.get();
        al_assert(tex);
        *tex = nullptr;
        RendererTexture2dHandle handle
        {
            .isValid = true,
            .index = static_cast<uint64_t>(texture2ds.get_direct_index(tex))
        };
        add_render_command([this, handle, path, cb]()
        {
            texture_2d(handle) = ::al::engine::internal::create_texture_2d<EngineConfig::DEFAULT_RENDERER_TYPE>(path);
            if (cb)
            {
                const_cast<Texture2dCallback*>(&cb)->call(handle);
            }
        });
        return handle;
    }

    inline Texture2d*& Renderer::texture_2d(RendererTexture2dHandle handle) noexcept
    {
        return *texture2ds.direct_accsess(static_cast<std::size_t>(handle.index));
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
                gpassShader = create_shader(vertGpassShaderStr, fragGpassShaderStr, [this, vertGpassShaderSrc, fragGpassShaderSrc](RendererShaderHandle handle)
                {
                    FileSystem::free_handle(vertGpassShaderSrc);
                    FileSystem::free_handle(fragGpassShaderSrc);
                    shader(handle)->bind();
                    shader(handle)->set_int(EngineConfig::DEFFERED_GEOMETRY_PASS_DIFFUSE_TEXTURE_NAME, EngineConfig::DEFFERED_GEOMETRY_PASS_DIFFUSE_TEXTURE_LOCATION);
                });
            }
            {
                FileHandle* vertDrawFramebufferToScreenShaderSrc = FileSystem::sync_load(EngineConfig::DRAW_FRAMEBUFFER_TO_SCREEN_VERT_SHADER_PATH, FileLoadMode::READ);
                FileHandle* fragDrawFramebufferToScreenShaderSrc = FileSystem::sync_load(EngineConfig::DRAW_FRAMEBUFFER_TO_SCREEN_FRAG_SHADER_PATH, FileLoadMode::READ);
                const char* vertDrawFramebufferToScreenShaderStr = reinterpret_cast<const char*>(vertDrawFramebufferToScreenShaderSrc->memory);
                const char* fragDrawFramebufferToScreenShaderStr = reinterpret_cast<const char*>(fragDrawFramebufferToScreenShaderSrc->memory);
                drawFramebufferToScreenShader = create_shader(vertDrawFramebufferToScreenShaderStr, fragDrawFramebufferToScreenShaderStr, 
                [vertDrawFramebufferToScreenShaderSrc, fragDrawFramebufferToScreenShaderSrc](RendererShaderHandle handle)
                {
                    FileSystem::free_handle(vertDrawFramebufferToScreenShaderSrc);
                    FileSystem::free_handle(fragDrawFramebufferToScreenShaderSrc);
                });
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
                screenRectangleVb = create_vertex_buffer(screenPlaneVertices, sizeof(screenPlaneVertices), [this](RendererVertexBufferHandle handle)
                {
                    vertex_buffer(handle)->set_layout(BufferLayout::ElementContainer{
                        BufferElement{ ShaderDataType::Float2, false }, // Position
                        BufferElement{ ShaderDataType::Float2, false }  // Uv
                    });
                });
                screenRectangleIb = create_index_buffer(screenPlaneIndices, sizeof(screenPlaneIndices) / sizeof(uint32_t));
                screenRectangleVa = create_vertex_array([this](RendererVertexArrayHandle handle)
                {
                    vertex_array(handle)->set_vertex_buffer(vertex_buffer(screenRectangleVb));
                    vertex_array(handle)->set_index_buffer(index_buffer(screenRectangleIb));
                });
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
            internal::destroy_framebuffer<EngineConfig::DEFAULT_RENDERER_TYPE>(framebuffer(gbuffer));
            internal::destroy_shader<EngineConfig::DEFAULT_RENDERER_TYPE>(shader(gpassShader));
            internal::destroy_shader<EngineConfig::DEFAULT_RENDERER_TYPE>(shader(drawFramebufferToScreenShader));
            internal::destroy_vertex_array<EngineConfig::DEFAULT_RENDERER_TYPE>(vertex_array(screenRectangleVa));
            internal::destroy_vertex_buffer<EngineConfig::DEFAULT_RENDERER_TYPE>(vertex_buffer(screenRectangleVb));
            internal::destroy_index_buffer<EngineConfig::DEFAULT_RENDERER_TYPE>(index_buffer(screenRectangleIb));
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

    namespace internal
    {
        std::size_t internal::get_max_renderer_size_bytes() noexcept
        {
            std::size_t result = 0;
            {
                std::size_t size = internal::get_renderer_size_bytes<RendererType::OPEN_GL>();
                result = size > result ? size : result;
            }
            return result;
        }
    }
}
