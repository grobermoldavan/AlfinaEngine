
#include "win32_opengl_renderer.h"

#include "engine/config/engine_config.h"

#include "engine/memory/memory_manager.h"

namespace al::engine
{
    template<> [[nodiscard]] Renderer* create_renderer<RendererType::OPEN_GL>(OsWindow* window) noexcept
    {
        Renderer* renderer = MemoryManager::get()->get_stack()->allocate_as<Win32OpenglRenderer>();
        ::new(renderer) Win32OpenglRenderer{ window };
        return renderer;
    }

    template<> void destroy_renderer<RendererType::OPEN_GL>(Renderer* renderer) noexcept
    {
        renderer->terminate();
        renderer->~Renderer();
        MemoryManager::get()->get_stack()->deallocate(reinterpret_cast<std::byte*>(renderer), sizeof(Win32OpenglRenderer));
    }

    void GLAPIENTRY OpenGlCallback( GLenum source,
                                    GLenum type,
                                    GLuint id,
                                    GLenum severity,
                                    GLsizei length,
                                    const GLchar* message,
                                    const void* userParam )
    {
        static constexpr const char* LOG_CATEGORY = "OpenGL";
        switch (severity)
        {
            case GL_DEBUG_SEVERITY_HIGH         : al_log_error(LOG_CATEGORY, "%s", message); break;
            case GL_DEBUG_SEVERITY_MEDIUM       : al_log_warning(LOG_CATEGORY, "%s", message); break;
            case GL_DEBUG_SEVERITY_LOW          : al_log_warning(LOG_CATEGORY, "%s", message); break;
            case GL_DEBUG_SEVERITY_NOTIFICATION : al_log_message(LOG_CATEGORY, "%s", message); break;
        }
    }

    Win32OpenglRenderer::Win32OpenglRenderer(OsWindow* window) noexcept
        : Renderer{ window }
        , win32window{ dynamic_cast<OsWindowWin32*>(window) }
    { }

    void Win32OpenglRenderer::clear_buffers() noexcept
    {
        ::glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        ::glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void Win32OpenglRenderer::swap_buffers() noexcept
    {
        ::SwapBuffers(deviceContext);
    }

    void Win32OpenglRenderer::draw(DrawCommandData* data) noexcept
    {
        data->va->bind();
        data->shader->bind();

        // For debug. Will be removed later
        uint32_t textureSlot = 0;
        data->texture->bind(textureSlot);

        data->shader->set_int("u_Texture", textureSlot);
        data->shader->set_mat4(EngineConfig::SHADER_MODEL_MATRIX_UNIFORM_NAME, data->trf.get_full_transform().transposed());
        data->shader->set_mat4(EngineConfig::SHADER_VIEW_PROJECTION_MATRIX_UNIFORM_NAME, (renderCamera->get_projection() * renderCamera->get_view()).transposed());

        ::glDrawElements(GL_TRIANGLES, data->va->get_index_buffer()->get_count(), GL_UNSIGNED_INT, nullptr);
    }

    void Win32OpenglRenderer::initialize_renderer() noexcept
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

        ::glEnable(GL_CULL_FACE);
        ::glCullFace(GL_BACK);

        // Vsync
        ::wglSwapIntervalEXT(1);

        ::glEnable(GL_DEBUG_OUTPUT);
        ::glDebugMessageCallback(OpenGlCallback, 0);
    }

    void Win32OpenglRenderer::terminate_renderer() noexcept
    {
        bool makeCurrentResult = ::wglMakeCurrent(deviceContext, NULL);
        al_assert(makeCurrentResult);

        bool deleteContextResult = ::wglDeleteContext(renderContext);
        al_assert(deleteContextResult);

        bool releaseDcResult = ::ReleaseDC(win32window->get_handle(), deviceContext);
        al_assert(releaseDcResult);
    }
}
