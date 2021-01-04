#ifndef AL_WIN32_OPENGL_RENDERER_H
#define AL_WIN32_OPENGL_RENDERER_H

#include "win32_opengl_backend.h"

#include "engine/rendering/renderer.h"
#include "win32_opengl_backend.h"
#include "win32_opengl_vertex_buffer.h"
#include "win32_opengl_index_buffer.h"
#include "win32_opengl_vertex_array.h"
#include "win32_opengl_shader.h"
#include "win32_opengl_texture_2d.h"
#include "win32_opengl_framebuffer.h"
#include "engine/platform/win32/window/os_window_win32.h"

namespace al::engine
{
    template<> [[nodiscard]] Renderer* create_renderer<RendererType::OPEN_GL>(OsWindow* window) noexcept;
    template<> void destroy_renderer<RendererType::OPEN_GL>(Renderer* renderer) noexcept;

    class Win32OpenglRenderer : public Renderer
    {
    public:
        Win32OpenglRenderer(OsWindow* window) noexcept;
        ~Win32OpenglRenderer() = default;

    protected:
        OsWindowWin32* win32window;
        HDC deviceContext;
        HGLRC renderContext;

        virtual void clear_buffers() noexcept override;
        virtual void swap_buffers() noexcept override;
        virtual void draw(VertexArray* va) noexcept override;
        virtual void initialize_renderer() noexcept override;
        virtual void terminate_renderer() noexcept override;
        virtual void bind_screen_framebuffer() noexcept override;
        virtual void set_depth_test_state(bool isEnabled) noexcept override;
    };
}

#endif
