#ifndef AL_WIN32_OPENGL_RENDERER_H
#define AL_WIN32_OPENGL_RENDERER_H

#include "engine/config/engine_config.h"
#include "win32_opengl_backend.h"

#include "../renderer.h"
#include "win32_opengl_backend.h"
#include "win32_opengl_vertex_buffer.h"
#include "win32_opengl_index_buffer.h"
#include "win32_opengl_vertex_array.h"
#include "win32_opengl_shader.h"
#include "engine/memory/memory_manager.h"
#include "engine/window/os_window_win32.h"
#include "engine/containers/dynamic_array.h"

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
            ::glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            ::glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }

        virtual void swap_buffers() noexcept override
        {
            ::SwapBuffers(deviceContext);
        }

        virtual void draw(VertexArray* va, Shader* shader, Transform* trf) noexcept override
        {
            va->bind();
            shader->bind();

            shader->set_mat4(EngineConfig::SHADER_MODEL_MATRIX_UNIFORM_NAME, trf->get_full_transform().transposed());
            shader->set_mat4(EngineConfig::SHADER_VIEW_PROJECTION_MATRIX_UNIFORM_NAME, (renderCamera->get_projection() * renderCamera->get_view()).transposed());
            ::glDrawElements(GL_TRIANGLES, va->get_index_buffer()->get_count(), GL_UNSIGNED_INT, nullptr);
        }

        virtual void initialize_renderer() noexcept override
        {
            deviceContext = ::GetDC(win32window->get_handle());
            al_assert(deviceContext);

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
            al_assert(renderContext); // Unable to create openGL context

            bool isCurrent = ::wglMakeCurrent(deviceContext, renderContext);
            al_assert(isCurrent); // Unable to make openGL context current

            GLenum glewInitRes = ::glewInit();
            al_assert(glewInitRes == GLEW_OK); // Unable to init GLEW

            ::wglMakeCurrent(deviceContext, renderContext);
            ::glEnable(GL_DEPTH_TEST);
            ::glDepthFunc(GL_LESS);

            // Vsync
            ::wglSwapIntervalEXT(1);
        }

        virtual void terminate_renderer() noexcept override
        {
            bool makeCurrentResult = ::wglMakeCurrent(deviceContext, NULL);
            al_assert(makeCurrentResult);

            bool deleteContextResult = ::wglDeleteContext(renderContext);
            al_assert(deleteContextResult);

            bool releaseDcResult = ::ReleaseDC(win32window->get_handle(), deviceContext);
            al_assert(releaseDcResult);
        }
    };

    template<>
    [[nodiscard]] Renderer* create_renderer<RendererType::OPEN_GL>(OsWindow* window) noexcept
    {
        Renderer* renderer = MemoryManager::get()->get_stack()->allocate_as<Win32OpenglRenderer>();
        ::new(renderer) Win32OpenglRenderer{ window };
        return renderer;
    }

    template<>
    void destroy_renderer<RendererType::OPEN_GL>(Renderer* renderer) noexcept
    {
        renderer->terminate();
        renderer->~Renderer();
        MemoryManager::get()->get_stack()->deallocate(reinterpret_cast<std::byte*>(renderer), sizeof(Win32OpenglRenderer));
    }
}

#endif
