#ifndef AL_RENDERER_H
#define AL_RENDERER_H

#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

#include "renderer_type.h"
#include "vertex_buffer.h"
#include "index_buffer.h"
#include "vertex_array.h"
#include "shader.h"
#include "draw_command_buffer.h"
#include "camera/render_camera.h"
#include "engine/debug/debug.h"
#include "engine/window/os_window.h"

#include "utilities/toggle.h"
#include "utilities/thread_event.h"
#include "utilities/function.h"
#include "utilities/thread_safe/thread_safe_only_growing_stack.h"
#include "utilities/math.h"

namespace al::engine
{
    class Renderer;

    template<RendererType type> [[nodiscard]] Renderer* create_renderer(OsWindow* window) noexcept; // @NOTE : this function is defined in renderer implementation header
    template<RendererType type> void destroy_renderer(Renderer* renderer) noexcept;                 // @NOTE : this function is defined in renderer implementation header

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

    class Renderer
    {
    public:
        using RenderCommand = Function<void()>;
        using RenderCommandBuffer = ThreadSafeOnlyGrowingStack<RenderCommand, EngineConfig::RENDER_COMMAND_STACK_SIZE>;

        Renderer(OsWindow* window) noexcept
            : renderThread{ &Renderer::render_update, this }
            , shouldRun{ true }
        { }

        ~Renderer() = default;

        void terminate() noexcept
        {
            // @NOTE :  Can't join render in the destructor because on joining thread will call
            //          virtual terminate_renderer method, but the child will be already terminated
            shouldRun = false;
            start_process_frame();
            al_exception_wrap(renderThread.join());
        }
        
        void start_process_frame() noexcept
        {
            al_assert(!onFrameProcessStart.is_invoked());
            onFrameProcessEnd.reset();
            onFrameProcessStart.invoke();
        }

        void wait_for_render_finish() noexcept
        {
            al_profile_function();

            const bool waitResult = onFrameProcessEnd.wait_for(std::chrono::seconds{ 1 });
            al_assert(waitResult); // Event was not fired. Probably something happend to render thread
        }

        void wait_for_command_buffers_toggled() noexcept
        {
            al_profile_function();
            
            const bool waitResult = onCommandBufferToggled.wait_for(std::chrono::seconds{ 1 });
            al_assert(waitResult); // Event was not fired. Probably something happend to render thread
            onCommandBufferToggled.reset();
        }

        void set_camera(const RenderCamera* camera) noexcept
        {
            renderCamera = camera;
        }

        void add_render_command(const RenderCommand& command) noexcept
        {
            RenderCommand* result = renderCommandBuffer.get_previous().push(command);
            al_assert(result);
        }

        [[nodiscard]] DrawCommandData* add_draw_command(DrawCommandKey key) noexcept
        {
            DrawCommandData* result = drawCommandBuffer.get_previous().add_command(key);
            al_assert(result);
            return result;
        }

    protected:
        static constexpr const char* LOG_CATEGORY_RENDERER = "Renderer";

        Toggle<RenderCommandBuffer> renderCommandBuffer;
        Toggle<DrawCommandBuffer> drawCommandBuffer;

        ThreadEvent onFrameProcessStart;
        ThreadEvent onFrameProcessEnd;
        ThreadEvent onCommandBufferToggled;

        std::thread renderThread;
        std::atomic<bool> shouldRun;

        const RenderCamera* renderCamera;

        virtual void clear_buffers() noexcept = 0;
        virtual void swap_buffers() noexcept = 0;
        virtual void draw(VertexArray* va, Shader* shader, Transform* trf) noexcept = 0;
        virtual void initialize_renderer() noexcept = 0;
        virtual void terminate_renderer() noexcept = 0;

        void render_update() noexcept
        {
            initialize_renderer();
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
                        drawCommandBuffer.toggle();
                    }
                    notify_command_buffers_toggled();

                    {
                        al_profile_scope("Clear buffers");
                        clear_buffers();
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
                        al_profile_scope("Process draw command buffer");
                        DrawCommandBuffer& current = drawCommandBuffer.get_current();
                        current.sort();
                        current.for_each([this](DrawCommandData* data)
                        {
                            if (!data->va)
                            {
                                al_log_error(LOG_CATEGORY_RENDERER, "Trying to process draw command, but vertex array is null");
                                return;
                            }

                            if (!data->shader)
                            {
                                al_log_error(LOG_CATEGORY_RENDERER, "Trying to process draw command, but shader is null");
                                return;
                            }

                            draw(data->va, data->shader, &data->trf);
                        });
                        current.clear();
                    }

                    {
                        al_profile_scope("Swap render buffers");
                        swap_buffers();
                    }
                }
                notify_render_finished();
            }
            terminate_renderer();
        }

        void wait_for_render_start() noexcept
        {
            al_profile_function();
            onFrameProcessStart.wait();
        }

        void notify_render_finished() noexcept
        {
            al_assert(!onFrameProcessEnd.is_invoked());
            onFrameProcessStart.reset();
            onFrameProcessEnd.invoke();
        }

        void notify_command_buffers_toggled() noexcept
        {
            al_assert(!onCommandBufferToggled.is_invoked());
            onCommandBufferToggled.invoke();
        }
    };
}

#ifdef AL_PLATFORM_WIN32
#   include "win32_opengl_rendering/win32_opengl_renderer.h"
#else
#   error Unsupported platform
#endif

#endif
