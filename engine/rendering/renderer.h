#ifndef AL_RENDERER_H
#define AL_RENDERER_H

#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

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
#include "utilities/thread_safe/thread_safe_only_growing_stack.h"
#include "utilities/math.h"
#include "utilities/static_unordered_list.h"

namespace al::engine
{
    struct Renderable
    {
        /* TODO : implement */
    };

    struct RendererHandleT
    {
        union
        {
            struct
            {
                uint64_t isValid : 1;
                uint64_t index : 63;
            };
            uint64_t value;
        };
    };

    using RendererIndexBufferHandle     = RendererHandleT;
    using RendererVertexBufferHandle    = RendererHandleT;
    using RendererVertexArrayHandle     = RendererHandleT;
    using RendererShaderHandle          = RendererHandleT;
    using RendererFramebufferHandle     = RendererHandleT;
    using RendererTexture2dHandle       = RendererHandleT;

    using IndexBufferCallback   = Function<void(RendererIndexBufferHandle)>;
    using VertexBufferCallback  = Function<void(RendererVertexBufferHandle)>;
    using VertexArrayCallback   = Function<void(RendererVertexArrayHandle)>;
    using ShaderCallback        = Function<void(RendererShaderHandle)>;
    using FramebufferCallback   = Function<void(RendererFramebufferHandle)>;
    using Texture2dCallback     = Function<void(RendererTexture2dHandle)>;

    class Renderer
    {
    public:
        using RenderCommand = Function<void(), 256>;
        using RenderCommandBuffer = ThreadSafeOnlyGrowingStack<RenderCommand, EngineConfig::RENDER_COMMAND_STACK_SIZE>;

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
        void add_render_command(const RenderCommand& command) noexcept;
        [[nodiscard]] GeometryCommandData* add_geometry_command(GeometryCommandKey key) noexcept;

        RendererIndexBufferHandle reserve_index_buffer() noexcept;
        void create_index_buffer(RendererIndexBufferHandle handle, uint32_t* indices, std::size_t count, IndexBufferCallback cb = IndexBufferCallback{ }) noexcept;
        void destroy_index_buffer(RendererIndexBufferHandle handle) noexcept;
        IndexBuffer*& index_buffer(RendererIndexBufferHandle handle) noexcept;

        RendererVertexBufferHandle reserve_vertex_buffer() noexcept;
        void create_vertex_buffer(RendererVertexBufferHandle handle, const void* data, std::size_t size, VertexBufferCallback cb = VertexBufferCallback{ }) noexcept;
        void destroy_vertex_buffer(RendererVertexBufferHandle handle) noexcept;
        VertexBuffer*& vertex_buffer(RendererVertexBufferHandle handle) noexcept;

        RendererVertexArrayHandle reserve_vertex_array() noexcept;
        void create_vertex_array(RendererVertexArrayHandle handle, VertexArrayCallback cb = VertexArrayCallback{ }) noexcept;
        void destroy_vertex_array(RendererVertexArrayHandle handle) noexcept;
        VertexArray*& vertex_array(RendererVertexArrayHandle handle) noexcept;

        RendererShaderHandle reserve_shader() noexcept;
        void create_shader(RendererShaderHandle handle, std::string_view vertexShaderSrc, std::string_view fragmentShaderSrc, ShaderCallback cb = ShaderCallback{ }) noexcept;
        void destroy_shader(RendererShaderHandle handle) noexcept;
        Shader*& shader(RendererShaderHandle handle) noexcept;

        RendererFramebufferHandle reserve_framebuffer() noexcept;
        void create_framebuffer(RendererFramebufferHandle handle, const FramebufferDescription& description, FramebufferCallback cb = FramebufferCallback{ }) noexcept;
        void destroy_framebuffer(RendererFramebufferHandle handle) noexcept;
        Framebuffer*& framebuffer(RendererFramebufferHandle handle) noexcept;

        RendererTexture2dHandle reserve_texture_2d() noexcept;
        void create_texture_2d(RendererTexture2dHandle handle, std::string_view path, Texture2dCallback cb = Texture2dCallback{ }) noexcept;
        void destroy_texture_2d(RendererTexture2dHandle handle) noexcept;
        Texture2d*& texture_2d(RendererTexture2dHandle handle) noexcept;

    protected:
        static Renderer* instance;

        static constexpr const char* LOG_CATEGORY_RENDERER = "Renderer";

        SuList<IndexBuffer* , EngineConfig::RENDERER_MAX_INDEX_BUFFERS>     indexBuffers;
        SuList<VertexBuffer*, EngineConfig::RENDERER_MAX_VERTEX_BUFFERS>    vertexBuffers;
        SuList<VertexArray* , EngineConfig::RENDERER_MAX_VERTEX_ARRAYS>     vertexArrays;
        SuList<Shader*      , EngineConfig::RENDERER_MAX_SHADERS>           shaders;
        SuList<Framebuffer* , EngineConfig::RENDERER_MAX_FRAMEBUFFERS>      framebuffers;
        SuList<Texture2d*   , EngineConfig::RENDERER_MAX_TEXTURES_2D>       texture2ds;

        Toggle<RenderCommandBuffer>     renderCommandBuffer;
        Toggle<GeometryCommandBuffer>   geometryCommandBuffer;

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
    };

    namespace internal
    {
        template<RendererType type>
        [[nodiscard]] void placement_create_renderer(Renderer* ptr, OsWindow* window) noexcept
        {
            // @NOTE :  This method is implemented at platform layer and it simply calls placement new
            //          with appropriate renderer type.
            al_log_error("Renderer", "Unsupported rendering API");
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
