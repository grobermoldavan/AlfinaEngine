
#include "renderer.h"

#include "engine/file_system/file_system.h"
#include "engine/job_system/job_system.h"

#include "utilities/safe_cast.h"

#define DECLARE_RENDERER_RESOURCE_ACTIONS_IMPLEMETATION(resourceNameSnakeCase, resourceNameCamelCase, resourcenameCamelCase2)   \
    Renderer##resourceNameCamelCase##Handle Renderer::reserve_##resourceNameSnakeCase() noexcept                                \
    {                                                                                                                           \
        al_profile_function();                                                                                                  \
        Renderer##resourceNameCamelCase##Handle handle;                                                                         \
        bool bufferHandleDequeueResult = free##resourceNameCamelCase##Handles.dequeue(&handle);                                 \
        al_assert(bufferHandleDequeueResult);                                                                                   \
        return handle;                                                                                                          \
    }                                                                                                                           \
    void Renderer::free_##resourceNameSnakeCase##_handle(Renderer##resourceNameCamelCase##Handle handle) noexcept               \
    {                                                                                                                           \
        al_profile_function();                                                                                                  \
        al_assert(handle.isValid);                                                                                              \
        bool bufferHandleEnqueueResult = free##resourceNameCamelCase##Handles.enqueue(&handle);                                 \
        al_assert(bufferHandleEnqueueResult);                                                                                   \
    }                                                                                                                           \
    RendererResourceActionResult Renderer::create_##resourceNameSnakeCase(                                                      \
                                                    Renderer##resourceNameCamelCase##Handle handle,                             \
                                                    const resourceNameCamelCase##InitData& initData) noexcept                   \
    {                                                                                                                           \
        al_profile_function();                                                                                                  \
        al_assert(handle.isValid);                                                                                              \
        if (is_render_thread())                                                                                                 \
        {                                                                                                                       \
            resourceNameSnakeCase(handle) =                                                                                     \
                internal::create_##resourceNameSnakeCase<EngineConfig::DEFAULT_RENDERER_TYPE>(initData);                        \
            return { true, nullptr };                                                                                           \
        }                                                                                                                       \
        else                                                                                                                    \
        {                                                                                                                       \
            Job* job = JobSystem::get_render_system()->get_job();                                                               \
            job->configure([this, handle, initData](Job*)                                                                       \
            {                                                                                                                   \
                resourceNameSnakeCase(handle) =                                                                                 \
                    internal::create_##resourceNameSnakeCase<EngineConfig::DEFAULT_RENDERER_TYPE>(initData);                    \
            });                                                                                                                 \
            JobSystem::get_render_system()->start_job(job);                                                                     \
            return { false, job };                                                                                              \
        }                                                                                                                       \
    }                                                                                                                           \
    RendererResourceActionResult Renderer::destroy_##resourceNameSnakeCase(                                                     \
                                                    Renderer##resourceNameCamelCase##Handle handle) noexcept                    \
    {                                                                                                                           \
        al_profile_function();                                                                                                  \
        al_assert(handle.isValid);                                                                                              \
        if (is_render_thread())                                                                                                 \
        {                                                                                                                       \
            internal::destroy_##resourceNameSnakeCase<EngineConfig::DEFAULT_RENDERER_TYPE>(                                     \
                resourceNameSnakeCase(handle));                                                                                 \
            resourceNameSnakeCase(handle) = nullptr;                                                                            \
            return { true, nullptr };                                                                                           \
        }                                                                                                                       \
        else                                                                                                                    \
        {                                                                                                                       \
            Job* job = JobSystem::get_render_system()->get_job();                                                               \
            job->configure([this, handle](Job*)                                                                                 \
            {                                                                                                                   \
                internal::destroy_##resourceNameSnakeCase<EngineConfig::DEFAULT_RENDERER_TYPE>(                                 \
                    resourceNameSnakeCase(handle));                                                                             \
                resourceNameSnakeCase(handle) = nullptr;                                                                        \
            });                                                                                                                 \
            JobSystem::get_render_system()->start_job(job);                                                                     \
            return { false, job };                                                                                              \
        }                                                                                                                       \
    }                                                                                                                           \
    inline resourceNameCamelCase*& Renderer::resourceNameSnakeCase(                                                             \
                                                        Renderer##resourceNameCamelCase##Handle handle) noexcept                \
    {                                                                                                                           \
        al_assert(handle.isValid);                                                                                              \
        return resourcenameCamelCase2##s[handle.index];                                                                         \
    }

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
        : renderThread{ }
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
        , indexBuffers{ 0 }
        , vertexBuffers{ 0 }
        , vertexArrays{ 0 }
        , shaders{ 0 }
        , texture2ds{ 0 }
        , freeIndexBufferHandles{ }
        , freeVertexBufferHandles{ }
        , freeVertexArrayHandles{ }
        , freeShaderHandles{ }
        , freeFramebufferHandles{ }
        , freeTexture2dHandles{ }
    {
        init_handle_queue(&freeIndexBufferHandles);
        init_handle_queue(&freeVertexBufferHandles);
        init_handle_queue(&freeVertexArrayHandles);
        init_handle_queue(&freeShaderHandles);
        init_handle_queue(&freeFramebufferHandles);
        init_handle_queue(&freeTexture2dHandles);
    }

    std::thread* Renderer::get_render_thread() noexcept
    {
        return &renderThread;
    }

    bool Renderer::is_render_thread() const noexcept
    {
        return std::this_thread::get_id() == renderThread.get_id();
    }

    void Renderer::terminate() noexcept
    {
        // @NOTE :  Can't join render in the destructor because on joining thread will call
        //          virtual terminate_renderer method, but the child will be already destroyed
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
        // al_assert(waitResult); // Event was not fired. Probably something happend to render thread
    }

    void Renderer::wait_for_command_buffers_toggled() noexcept
    {
        al_profile_function();
        const bool waitResult = onCommandBufferToggled->wait_for(std::chrono::seconds{ 1 });
        // al_assert(waitResult); // Event was not fired. Probably something happend to render thread
        onCommandBufferToggled->reset();
    }

    void Renderer::set_camera(const RenderCamera* camera) noexcept
    {
        renderCamera = camera;
    }

    [[nodiscard]] GeometryCommandData* Renderer::add_geometry_command(GeometryCommandKey key) noexcept
    {
        GeometryCommandData* result = geometryCommandBuffer.get_previous().add_command(key);
        al_assert(result);
        return result;
    }

    DECLARE_RENDERER_RESOURCE_ACTIONS_IMPLEMETATION(index_buffer    , IndexBuffer   , indexBuffer);
    DECLARE_RENDERER_RESOURCE_ACTIONS_IMPLEMETATION(vertex_buffer   , VertexBuffer  , vertexBuffer);
    DECLARE_RENDERER_RESOURCE_ACTIONS_IMPLEMETATION(vertex_array    , VertexArray   , vertexArray);
    DECLARE_RENDERER_RESOURCE_ACTIONS_IMPLEMETATION(shader          , Shader        , shader);
    DECLARE_RENDERER_RESOURCE_ACTIONS_IMPLEMETATION(framebuffer     , Framebuffer   , framebuffer);
    DECLARE_RENDERER_RESOURCE_ACTIONS_IMPLEMETATION(texture_2d      , Texture2d     , texture2d);

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
                gbuffer = reserve_framebuffer();
                create_framebuffer(gbuffer, { gbufferDesciption });
            }
            {
                FileHandle* vertGpassShaderSrc = FileSystem::get()->sync_load(EngineConfig::DEFFERED_GEOMETRY_PASS_VERT_SHADER_PATH, FileLoadMode::READ);
                FileHandle* fragGpassShaderSrc = FileSystem::get()->sync_load(EngineConfig::DEFFERED_GEOMETRY_PASS_FRAG_SHADER_PATH, FileLoadMode::READ);
                const char* vertGpassShaderStr = reinterpret_cast<const char*>(vertGpassShaderSrc->memory);
                const char* fragGpassShaderStr = reinterpret_cast<const char*>(fragGpassShaderSrc->memory);
                gpassShader = reserve_shader();
                create_shader(gpassShader, { vertGpassShaderStr, fragGpassShaderStr });
                FileSystem::get()->free_handle(vertGpassShaderSrc);
                FileSystem::get()->free_handle(fragGpassShaderSrc);
                shader(gpassShader)->bind();
                shader(gpassShader)->set_int(EngineConfig::DEFFERED_GEOMETRY_PASS_DIFFUSE_TEXTURE_NAME, EngineConfig::DEFFERED_GEOMETRY_PASS_DIFFUSE_TEXTURE_LOCATION);
            }
            {
                FileHandle* vertDrawFramebufferToScreenShaderSrc = FileSystem::get()->sync_load(EngineConfig::DRAW_FRAMEBUFFER_TO_SCREEN_VERT_SHADER_PATH, FileLoadMode::READ);
                FileHandle* fragDrawFramebufferToScreenShaderSrc = FileSystem::get()->sync_load(EngineConfig::DRAW_FRAMEBUFFER_TO_SCREEN_FRAG_SHADER_PATH, FileLoadMode::READ);
                const char* vertDrawFramebufferToScreenShaderStr = reinterpret_cast<const char*>(vertDrawFramebufferToScreenShaderSrc->memory);
                const char* fragDrawFramebufferToScreenShaderStr = reinterpret_cast<const char*>(fragDrawFramebufferToScreenShaderSrc->memory);
                drawFramebufferToScreenShader = reserve_shader();
                create_shader(drawFramebufferToScreenShader, { vertDrawFramebufferToScreenShaderStr, fragDrawFramebufferToScreenShaderStr });
                FileSystem::get()->free_handle(vertDrawFramebufferToScreenShaderSrc);
                FileSystem::get()->free_handle(fragDrawFramebufferToScreenShaderSrc);
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
                screenRectangleVb = reserve_vertex_buffer();
                create_vertex_buffer(screenRectangleVb, { screenPlaneVertices, sizeof(screenPlaneVertices) });
                vertex_buffer(screenRectangleVb)->set_layout(BufferLayout::ElementContainer{
                    BufferElement{ ShaderDataType::Float2, false }, // Position
                    BufferElement{ ShaderDataType::Float2, false }  // Uv
                });
                screenRectangleIb = reserve_index_buffer();
                create_index_buffer(screenRectangleIb, { screenPlaneIndices, sizeof(screenPlaneIndices) / sizeof(uint32_t) });
                screenRectangleVa = reserve_vertex_array();
                create_vertex_array(screenRectangleVa, { });
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
                    geometryCommandBuffer.toggle();
                    notify_command_buffers_toggled();
                }
                {
                    al_profile_scope("Process render jobs");
                    Job* job = JobSystem::get_render_system()->get_job_from_queue();
                    while (job)
                    {
                        job->dispatch();
                        job = JobSystem::get_render_system()->get_job_from_queue();
                    }
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
                            al_log_error(EngineConfig::RENDERER_LOG_CATEGORY, "Trying to process draw command, but vertex array is null");
                            return;
                        }
                        if (!data->diffuseTexture)
                        {
                            al_log_error(EngineConfig::RENDERER_LOG_CATEGORY, "Trying to process draw command, but diffuse texture is null");
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
            destroy_framebuffer(gbuffer);
            destroy_shader(gpassShader);
            destroy_shader(drawFramebufferToScreenShader);
            destroy_vertex_array(screenRectangleVa);
            destroy_vertex_buffer(screenRectangleVb);
            destroy_index_buffer(screenRectangleIb);
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

    void Renderer::start_render_thread() noexcept
    {
        renderThread = std::thread{ &Renderer::render_update, this };
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
