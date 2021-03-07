
#include "win32_opengl_renderer.h"

#include "engine/config/engine_config.h"

#include "engine/memory/memory_manager.h"

namespace al::engine
{
    void GLAPIENTRY OpenGlCallback( GLenum source,
                                    GLenum type,
                                    GLuint id,
                                    GLenum severity,
                                    GLsizei length,
                                    const GLchar* message,
                                    const void* userParam )
    {
        switch (severity)
        {
            case GL_DEBUG_SEVERITY_HIGH         : al_log_error(EngineConfig::OPENGL_RENDERER_LOG_CATEGORY, "%s", message); break;
            case GL_DEBUG_SEVERITY_MEDIUM       : al_log_warning(EngineConfig::OPENGL_RENDERER_LOG_CATEGORY, "%s", message); break;
            case GL_DEBUG_SEVERITY_LOW          : al_log_warning(EngineConfig::OPENGL_RENDERER_LOG_CATEGORY, "%s", message); break;
            case GL_DEBUG_SEVERITY_NOTIFICATION : al_log_message(EngineConfig::OPENGL_RENDERER_LOG_CATEGORY, "%s", message); break;
        }
    }

    Win32OpenglRenderer::Win32OpenglRenderer(OsWindow* window) noexcept
        : Renderer{ window }
        , win32window{ dynamic_cast<OsWindowWin32*>(window) }
    {
        al_profile_function();
        start_render_thread();
    }

    void Win32OpenglRenderer::clear_buffers() noexcept
    {
        ::glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void Win32OpenglRenderer::swap_buffers() noexcept
    {
        ::SwapBuffers(deviceContext);
    }

    void Win32OpenglRenderer::draw(VertexArray* va) noexcept
    {
        al_profile_function();
        va->bind();
        ::glDrawElements(GL_TRIANGLES, va->get_index_buffer()->get_count(), GL_UNSIGNED_INT, nullptr);
    }

    void Win32OpenglRenderer::initialize_renderer() noexcept
    {
        al_profile_function();
        deviceContext = ::GetDC(win32window->get_handle());
        al_assert(deviceContext);
        HGLRC dummyRenderContext;
        {
            // @NOTE :  Creating dummy context
            //          This is done in order to call wgl functions and create propper rendering context
            //          https://www.khronos.org/opengl/wiki/Creating_an_OpenGL_Context_(WGL)
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

            int32_t pixelFormat = ::ChoosePixelFormat(deviceContext, &pfd);
            al_assert(pixelFormat != 0) // Unable to choose pixel format for a given descriptor

            bool isPixelFormatSet = ::SetPixelFormat(deviceContext, pixelFormat, &pfd);
            al_assert(isPixelFormatSet); // Unable to set pixel format

            dummyRenderContext = ::wglCreateContext(deviceContext);
            al_assert(dummyRenderContext); // Unable to create dummy OpenGL context

            bool isCurrent = ::wglMakeCurrent(deviceContext, dummyRenderContext);
            al_assert(isCurrent); // Unable to make OpenGL context current
        }
        {
            // @NOTE :  Init GLEW, check for extensions and create actual rendering context.
            GLenum glewInitRes = ::glewInit();
            al_assert(glewInitRes == GLEW_OK); // Unable to init GLEW

            // @TODO :  Maybe should handle situations where these extensions are not supported.
            // @NOTE :  Must support this extension
            //          This extension is needed for wglChoosePixelFormatARB function
            al_assert(::wglewIsSupported("WGL_ARB_pixel_format"));
            // @NOTE :  Must support this extension
            //          This extension is needed for wglCreateContextAttribsARB function
            al_assert(::wglewIsSupported("WGL_ARB_create_context"));
            // @NOTE :  Must support this extension
            //          This extension is needed for wglSwapIntervalEXT - vsync enable/disable function
            al_assert(::wglewIsSupported("WGL_EXT_swap_control"));

            const int32_t integerChoosePixelFormatAttributes[] =
            {
                WGL_DRAW_TO_WINDOW_ARB  , GL_TRUE,
                WGL_SUPPORT_OPENGL_ARB  , GL_TRUE,
                WGL_DOUBLE_BUFFER_ARB   , GL_TRUE,
                WGL_PIXEL_TYPE_ARB      , WGL_TYPE_RGBA_ARB,
                WGL_COLOR_BITS_ARB      , 32,
                WGL_DEPTH_BITS_ARB      , 24,
                WGL_STENCIL_BITS_ARB    , 8,
                0                       , // End
            };

            int32_t pixelFormat;
            uint32_t numberOfFormats;
            bool choosePixelFormatResult = ::wglChoosePixelFormatARB(deviceContext, integerChoosePixelFormatAttributes, nullptr, 1, &pixelFormat, &numberOfFormats);
            al_assert(choosePixelFormatResult); // Unable to choose pixel format

            PIXELFORMATDESCRIPTOR pfd;
            ::DescribePixelFormat(deviceContext, pixelFormat, sizeof(pfd), &pfd);

            bool isPixelFormatSet = ::SetPixelFormat(deviceContext, pixelFormat, &pfd);
            al_assert(isPixelFormatSet); // Unable to set pixel format

            const int32_t integerCreateContextAttributes[] =
            {
                WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB, // Use core profile
                0                           , // End
            };

            renderContext = ::wglCreateContextAttribsARB(deviceContext, nullptr, integerCreateContextAttributes);
            al_assert(renderContext); // Unable to create actual OpenGL context

            bool isCurrent = ::wglMakeCurrent(deviceContext, renderContext);
            al_assert(isCurrent); // Unable to make OpenGL context current

            al_log_message(EngineConfig::OPENGL_RENDERER_LOG_CATEGORY, "OpenGL version : %s", ::glGetString(GL_VERSION));
        }
        {
            // @NOTE :  Delete dummy context
            bool deleteContextResult = ::wglDeleteContext(dummyRenderContext);
            al_assert(deleteContextResult);
        }
        // Enable depth testing
        ::glEnable(GL_DEPTH_TEST);
        ::glDepthFunc(GL_LESS);
        // Enable backface culling
        ::glEnable(GL_CULL_FACE);
        ::glCullFace(GL_BACK);
        // Enable debug output
        ::glEnable(GL_DEBUG_OUTPUT);
        ::glDebugMessageCallback(OpenGlCallback, 0);
        // Set clear color
        ::glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    }

    void Win32OpenglRenderer::terminate_renderer() noexcept
    {
        al_profile_function();
        bool makeCurrentResult = ::wglMakeCurrent(deviceContext, NULL);
        al_assert(makeCurrentResult);
        bool deleteContextResult = ::wglDeleteContext(renderContext);
        al_assert(deleteContextResult);
        bool releaseDcResult = ::ReleaseDC(win32window->get_handle(), deviceContext);
        al_assert(releaseDcResult);
    }

    void Win32OpenglRenderer::bind_screen_framebuffer() noexcept
    {
        ::glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void Win32OpenglRenderer::set_depth_test_state(bool isEnabled) noexcept
    {
        if (isEnabled)
        {
            ::glEnable(GL_DEPTH_TEST);
        }
        else
        {
            ::glDisable(GL_DEPTH_TEST);
        }
    }

    void Win32OpenglRenderer::set_vsync_state(bool isEnabled) noexcept
    {
        ::wglSwapIntervalEXT(isEnabled ? 1 : 0);
    }

    namespace internal
    {
        template<> void internal::placement_create_renderer<RendererType::OPEN_GL>(Renderer* ptr, OsWindow* window) noexcept
        {
            ::new(ptr) Win32OpenglRenderer{ window };
        }

        template<> std::size_t internal::get_renderer_size_bytes<RendererType::OPEN_GL>() noexcept
        {
            return sizeof(Win32OpenglRenderer);
        }
    }
}
