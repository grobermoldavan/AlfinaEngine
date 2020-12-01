#ifndef AL_RENDERER_H
#define AL_RENDERER_H

#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

#include "vertex_buffer.h"
#include "index_buffer.h"
#include "vertex_array.h"
#include "draw_command_buffer.h"
#include "camera/perspective_camera.h"
#include "engine/debug/debug.h"
#include "engine/memory/allocator_base.h"
#include "engine/window/os_window.h"

#include "utilities/toggle.h"
#include "utilities/function.h"

namespace al::engine
{
    class Renderer;

    [[nodiscard]] Renderer* create_renderer(OsWindow* window, AllocatorBase* allocator);    // @NOTE : this function is defined in renderer implementation header
    void destroy_renderer(Renderer* renderer, AllocatorBase* allocator);                    // @NOTE : this function is defined in renderer implementation header

    class Renderer
    {
    public:
        Renderer(OsWindow* window) noexcept
            : renderThread{ &Renderer::render_update, this }
            , shouldRun{ true }
            , isProcessingFrame{ false } 
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
            al_assert(!isProcessingFrame);
            isProcessingFrame = true;
            al_exception_wrap(processingCv.notify_one());
        }

        void wait_for_render_finish() noexcept
        {
            if (isProcessingFrame)
            {
                std::unique_lock<std::mutex> lock{ processingMutex };
                al_exception_wrap(processingCv.wait(lock, [this]() -> bool { return !isProcessingFrame; }));
            }
        }

        void set_camera(const PerspectiveRenderCamera& camera) noexcept
        {
            renderCamera = camera;
        }

    protected:
        Toggle<DrawCommandBuffer> drawCommandBuffer;
        std::thread renderThread;
        std::atomic<bool> shouldRun;

        bool isProcessingFrame;
        std::mutex processingMutex;
        std::condition_variable processingCv;

        PerspectiveRenderCamera renderCamera;

        virtual void clear_buffers() noexcept = 0;
        virtual void swap_buffers() noexcept = 0;
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

                    clear_buffers();

                    DrawCommandBuffer& current = drawCommandBuffer.get_current();
                    current.sort();
                    current.for_each([&](DrawCommandData* data)
                    {
                        // draw object
                    });
                    drawCommandBuffer.toggle();

                    swap_buffers();
                }
                notify_render_finished();
            }
            terminate_renderer();
        }

        void wait_for_render_start() noexcept
        {
            if (!isProcessingFrame)
            {
                std::unique_lock<std::mutex> lock{ processingMutex };
                al_exception_wrap(processingCv.wait(lock, [this]() -> bool { return isProcessingFrame; }));
            }
        }

        void notify_render_finished() noexcept
        {
            al_assert(isProcessingFrame);
            isProcessingFrame = false;
            al_exception_wrap(processingCv.notify_one());
        }
    };
}

#ifdef AL_PLATFORM_WIN32
#   ifdef AL_OPENGL
#       include "win32_opengl_rendering/win32_opengl_renderer.h"
#   else
#       error Usupproted windows graphics api
#   endif
#else
#   error Unsupported platform
#endif

#endif
