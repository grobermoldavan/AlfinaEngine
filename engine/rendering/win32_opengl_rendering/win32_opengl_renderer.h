#ifndef AL_WIN32_OPENGL_RENDERER_H
#define AL_WIN32_OPENGL_RENDERER_H

#include "../renderer.h"
#include "win32_opengl_backend.h"
#include "win32_opengl_vertex_buffer.h"
#include "win32_opengl_index_buffer.h"
#include "win32_opengl_vertex_array.h"
#include "engine/window/os_window_win32.h"

namespace al::engine
{
    class Win32OpenglRenderer : public Renderer
    {
    public:
        Win32OpenglRenderer(OsWindow* window) noexcept
            : Renderer{ window }
            , win32window{ dynamic_cast<OsWindowWin32*>(window) }
        { }

        ~Win32OpenglRenderer() = default;

    protected:
        OsWindowWin32* win32window;
        HDC deviceContext;
        HGLRC renderContext;

        virtual void clear_buffers() noexcept override
        {
            ::glClearColor(1, 0, 0, 1.0f);
            ::glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }

        virtual void swap_buffers() noexcept override
        {
            ::SwapBuffers(deviceContext);
        }

        virtual void initialize_renderer() noexcept override
        {
            deviceContext = ::GetDC(win32window->get_handle());
            al_assert(deviceContext) // unable to retrieve device context

            PIXELFORMATDESCRIPTOR pfd
            {
                sizeof(PIXELFORMATDESCRIPTOR),
                1,
                PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, // Flags
                PFD_TYPE_RGBA,                                              // The kind of framebuffer. RGBA or palette.
                32,                                                         // Colordepth of the framebuffer.
                0, 0, 0, 0, 0, 0,
                0,
                0,
                0,
                0, 0, 0, 0,
                24,                                                         // Number of bits for the depthbuffer
                8,                                                          // Number of bits for the stencilbuffer
                0,                                                          // Number of Aux buffers in the framebuffer.
                PFD_MAIN_PLANE,
                0,
                0, 0, 0
            };

            int pixelFormat = ::ChoosePixelFormat(deviceContext, &pfd);
            al_assert(pixelFormat != 0) // Unable to choose pixel format for a given descriptor

            bool isPixelFormatSet = ::SetPixelFormat(deviceContext, pixelFormat, &pfd);
            al_assert(isPixelFormatSet); // Unable to set pixel format

            renderContext = ::wglCreateContext(deviceContext);
            al_assert(renderContext); // Unable to create openGL context")

            bool isCurrent = ::wglMakeCurrent(deviceContext, renderContext);
            al_assert(isCurrent); // Unable to make openGL context current")

            GLenum glewInitRes = ::glewInit();
            al_assert(glewInitRes == GLEW_OK); // Unable to init GLEW")

            ::wglMakeCurrent(deviceContext, renderContext);
            ::glEnable(GL_DEPTH_TEST);
            ::glDepthFunc(GL_LESS);

            // Vsync
            ::wglSwapIntervalEXT(1);
        }

        virtual void terminate_renderer() noexcept override
        {
            ::wglMakeCurrent(deviceContext, NULL);
            ::wglDeleteContext(renderContext);
            ::ReleaseDC(win32window->get_handle(), deviceContext);
        }
    };

    [[nodiscard]] Renderer* create_renderer(OsWindow* window, AllocatorBase* allocator)
    {
        Renderer* renderer = allocator->allocate_as<Win32OpenglRenderer>();
        ::new(renderer) Win32OpenglRenderer{ window };
        return renderer;
    }

    void destroy_renderer(Renderer* renderer, AllocatorBase* allocator)
    {
        renderer->terminate();
        renderer->~Renderer();
        allocator->deallocate(reinterpret_cast<std::byte*>(renderer), sizeof(Win32OpenglRenderer));
    }
}

#endif
