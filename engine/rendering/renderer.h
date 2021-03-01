#ifndef AL_RENDERER_H
#define AL_RENDERER_H

#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <cstdint>

#include "render_core.h"
#include "vertex_buffer.h"
#include "index_buffer.h"
#include "vertex_array.h"
#include "shader.h"
#include "framebuffer.h"
#include "geometry_command_buffer.h"
#include "texture_2d.h"
#include "engine/rendering/camera/render_camera.h"
#include "engine/debug/debug.h"
#include "engine/window/os_window.h"
#include "engine/platform/platform_thread_event.h"
#include "engine/containers/containers.h"

#include "utilities/toggle.h"
#include "utilities/function.h"
#include "utilities/thread_safe/thread_safe_queue.h"
#include "utilities/math.h"
#include "utilities/static_unordered_list.h"

#define DECLARE_RENDERER_RESOURCE_ACTIONS(resourceNameSnakeCase, resourceNameCamelCase)                                                                                         \
        Renderer##resourceNameCamelCase##Handle reserve_##resourceNameSnakeCase() noexcept;                                                                                     \
        void free_##resourceNameSnakeCase##_handle(Renderer##resourceNameCamelCase##Handle handle) noexcept;                                                                    \
        RendererResourceActionResult create_##resourceNameSnakeCase(Renderer##resourceNameCamelCase##Handle handle, const resourceNameCamelCase##InitData& initData) noexcept;  \
        RendererResourceActionResult destroy_##resourceNameSnakeCase(Renderer##resourceNameCamelCase##Handle handle) noexcept;                                                  \
        resourceNameCamelCase*& resourceNameSnakeCase(Renderer##resourceNameCamelCase##Handle handle) noexcept;

namespace al::engine
{
    struct RendererResourceActionResult
    {
        bool isAlreadyDone;
        Job* job;
    };

    class Renderer
    {
    public:
        static void allocate_space() noexcept;
        static void construct(RendererType type, OsWindow* window) noexcept;
        static void destruct() noexcept;
        static Renderer* get() noexcept;

        Renderer(OsWindow* window) noexcept;
        ~Renderer() = default;

        std::thread* get_render_thread() noexcept;
        bool is_render_thread() const noexcept;

        void terminate() noexcept;
        void start_process_frame() noexcept;
        void wait_for_render_finish() noexcept;
        void wait_for_command_buffers_toggled() noexcept;
        void set_camera(const RenderCamera* camera) noexcept;
        [[nodiscard]] GeometryCommandData* add_geometry_command(GeometryCommandKey key) noexcept;

        DECLARE_RENDERER_RESOURCE_ACTIONS(index_buffer   , IndexBuffer);
        DECLARE_RENDERER_RESOURCE_ACTIONS(vertex_buffer  , VertexBuffer);
        DECLARE_RENDERER_RESOURCE_ACTIONS(vertex_array   , VertexArray);
        DECLARE_RENDERER_RESOURCE_ACTIONS(shader         , Shader);
        DECLARE_RENDERER_RESOURCE_ACTIONS(framebuffer    , Framebuffer);
        DECLARE_RENDERER_RESOURCE_ACTIONS(texture_2d     , Texture2d);

    protected:
        static Renderer* instance;

        // @TODO :  Maybe somehow move this info into framebuffer description ?
        static constexpr uint32_t GBUFFER_POSITION_ATTACHMENT   = 0;
        static constexpr uint32_t GBUFFER_NORMAL_ATTACHMENT     = 1;
        static constexpr uint32_t GBUFFER_DIFFUSE_ATTACHMENT    = 2;

        IndexBuffer*    indexBuffers    [EngineConfig::RENDERER_MAX_INDEX_BUFFERS];
        VertexBuffer*   vertexBuffers   [EngineConfig::RENDERER_MAX_VERTEX_BUFFERS];
        VertexArray*    vertexArrays    [EngineConfig::RENDERER_MAX_VERTEX_ARRAYS];
        Shader*         shaders         [EngineConfig::RENDERER_MAX_SHADERS];
        Framebuffer*    framebuffers    [EngineConfig::RENDERER_MAX_FRAMEBUFFERS];
        Texture2d*      texture2ds      [EngineConfig::RENDERER_MAX_TEXTURES_2D];

        StaticThreadSafeQueue<RendererIndexBufferHandle , EngineConfig::RENDERER_MAX_INDEX_BUFFERS>     freeIndexBufferHandles;
        StaticThreadSafeQueue<RendererVertexBufferHandle, EngineConfig::RENDERER_MAX_VERTEX_BUFFERS>    freeVertexBufferHandles;
        StaticThreadSafeQueue<RendererVertexArrayHandle , EngineConfig::RENDERER_MAX_VERTEX_ARRAYS>     freeVertexArrayHandles;
        StaticThreadSafeQueue<RendererShaderHandle      , EngineConfig::RENDERER_MAX_SHADERS>           freeShaderHandles;
        StaticThreadSafeQueue<RendererFramebufferHandle , EngineConfig::RENDERER_MAX_FRAMEBUFFERS>      freeFramebufferHandles;
        StaticThreadSafeQueue<RendererTexture2dHandle   , EngineConfig::RENDERER_MAX_TEXTURES_2D>       freeTexture2dHandles;

        Toggle<GeometryCommandBuffer> geometryCommandBuffer;

        ThreadEvent* onFrameProcessStart;
        ThreadEvent* onFrameProcessEnd;
        ThreadEvent* onCommandBufferToggled;

        std::thread renderThread;
        std::atomic<bool> shouldRun;

        OsWindow* window;

        const RenderCamera* renderCamera;

        RendererShaderHandle gpassShader;
        RendererFramebufferHandle gbuffer;        

        RendererShaderHandle drawFramebufferToScreenShader;
        RendererVertexBufferHandle screenRectangleVb;
        RendererIndexBufferHandle screenRectangleIb;
        RendererVertexArrayHandle screenRectangleVa;

        virtual void clear_buffers() noexcept = 0;
        virtual void swap_buffers() noexcept = 0;
        virtual void draw(VertexArray* va) noexcept = 0;
        virtual void initialize_renderer() noexcept = 0;
        virtual void terminate_renderer() noexcept = 0;
        virtual void bind_screen_framebuffer() noexcept = 0;
        virtual void set_depth_test_state(bool isEnabled) noexcept = 0;
        virtual void set_vsync_state(bool isEnabled) noexcept = 0;

        void render_update() noexcept;
        void wait_for_render_start() noexcept;
        void notify_render_finished() noexcept;
        void notify_command_buffers_toggled() noexcept;
        void start_render_thread() noexcept;

        template<typename HandleT, std::size_t NumHandles>
        void init_handle_queue(StaticThreadSafeQueue<HandleT, NumHandles>* queue)
        {
            for (std::size_t it = 0; it < NumHandles; it++)
            {
                HandleT handle
                {
                    .isValid = 1,
                    .index = it
                };
                queue->enqueue(&handle);
            }
        }
    };

    namespace internal
    {
        template<RendererType type>
        [[nodiscard]] void placement_create_renderer(Renderer* ptr, OsWindow* window) noexcept
        {
            // @NOTE :  This method is implemented at platform layer and it simply calls placement new
            //          with appropriate renderer type.
            al_log_error(EngineConfig::RENDERER_LOG_CATEGORY, "Unsupported rendering API");
            al_assert(false);
            return nullptr;
        }

        template<RendererType type>
        std::size_t get_renderer_size_bytes() noexcept
        {
            // @NOTE :  This method is implemented at platform layer and it simply returns
            //          size of appropriate renderer type.
            return 0;
        }

        std::size_t get_max_renderer_size_bytes() noexcept;
    }
}

#endif
