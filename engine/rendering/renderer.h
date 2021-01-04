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

#include "utilities/toggle.h"
#include "utilities/function.h"
#include "utilities/thread_safe/thread_safe_only_growing_stack.h"
#include "utilities/math.h"

namespace al::engine
{
    class Renderer
    {
    public:
        using RenderCommand = Function<void()>;
        using RenderCommandBuffer = ThreadSafeOnlyGrowingStack<RenderCommand, EngineConfig::RENDER_COMMAND_STACK_SIZE>;

        Renderer(OsWindow* window) noexcept;
        ~Renderer() = default;

        void terminate() noexcept;
        void start_process_frame() noexcept;
        void wait_for_render_finish() noexcept;
        void wait_for_command_buffers_toggled() noexcept;
        void set_camera(const RenderCamera* camera) noexcept;
        void add_render_command(const RenderCommand& command) noexcept;
        [[nodiscard]] GeometryCommandData* add_geometry(GeometryCommandKey key) noexcept;

    protected:
        static constexpr const char* LOG_CATEGORY_RENDERER = "Renderer";

        Toggle<RenderCommandBuffer> renderCommandBuffer;
        Toggle<GeometryCommandBuffer> geometryCommandBuffer;

        ThreadEvent* onFrameProcessStart;
        ThreadEvent* onFrameProcessEnd;
        ThreadEvent* onCommandBufferToggled;

        std::thread renderThread;
        std::atomic<bool> shouldRun;

        OsWindow* window;

        const RenderCamera* renderCamera;

        Shader* gpassShader;
        Framebuffer* gbuffer;        

        Shader* drawFramebufferToScreenShader;
        VertexBuffer* screenRectangleVb;
        IndexBuffer* screenRectangleIb;
        VertexArray* screenRectangleVa;

        virtual void clear_buffers() noexcept = 0;
        virtual void swap_buffers() noexcept = 0;
        virtual void draw(VertexArray* va) noexcept = 0;
        virtual void initialize_renderer() noexcept = 0;
        virtual void terminate_renderer() noexcept = 0;
        virtual void bind_screen_framebuffer() noexcept = 0;
        virtual void set_depth_test_state(bool isEnabled) noexcept = 0;

        void render_update() noexcept;
        void wait_for_render_start() noexcept;
        void notify_render_finished() noexcept;
        void notify_command_buffers_toggled() noexcept;
    };

    template<RendererType type>
    [[nodiscard]] Renderer* create_renderer(OsWindow* window) noexcept
    {
        al_log_error("Renderer", "Unsupported rendering API");
        al_assert(false);
        return nullptr;
    }

    template<RendererType type>
    void destroy_renderer(Renderer* renderer) noexcept
    {
        al_log_error("Renderer", "Unsupported rendering API");
        al_assert(false);
    }
}

#endif
