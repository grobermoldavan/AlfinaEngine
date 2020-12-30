
#include "renderer.h"

namespace al::engine
{
    Renderer::Renderer(OsWindow* window) noexcept
        : renderThread{ &Renderer::render_update, this }
        , shouldRun{ true }
        , window{ window }
        , onFrameProcessStart{ create_thread_event() }
        , onFrameProcessEnd{ create_thread_event() }
        , onCommandBufferToggled{ create_thread_event() }
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

    [[nodiscard]] DrawCommandData* Renderer::add_draw_command(DrawCommandKey key) noexcept
    {
        DrawCommandData* result = drawCommandBuffer.get_previous().add_command(key);
        al_assert(result);
        return result;
    }

    void Renderer::render_update() noexcept
    {
        initialize_renderer();

        {
            al_profile_scope("Renderer post-init");

            FramebufferDescription desc;
            desc.attachments = { FramebufferAttachmentType::RGB_8, FramebufferAttachmentType::DEPTH_24_STENCIL_8 };
            desc.width = window->get_params()->width;
            desc.height = window->get_params()->height;
            defaultFramebuffer = create_framebuffer<EngineConfig::DEFAULT_RENDERER_TYPE>(desc);
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
                        if (!data->texture)
                        {
                            al_log_error(LOG_CATEGORY_RENDERER, "Trying to process draw command, but texture is null");
                            return;
                        }
                        draw(data);
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
