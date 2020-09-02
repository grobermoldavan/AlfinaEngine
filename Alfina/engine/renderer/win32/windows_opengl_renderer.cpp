#if defined(AL_UNITY_BUILD)

#else
#	include "windows_opengl_renderer.h"
#endif

#include "engine/engine_utilities/asserts.h"

namespace al::engine
{
	ErrorInfo create_renderer(Renderer** renderer, ApplicationWindow* window)
	{
		*renderer = static_cast<Renderer*>(new Win32glRenderer(static_cast<Win32ApplicationWindow*>(window)));
		return{ ErrorInfo::Code::ALL_FINE };
	}

	ErrorInfo destroy_renderer(Renderer* renderer)
	{
		if (renderer) delete renderer;
		return{ ErrorInfo::Code::ALL_FINE };
	}

	Win32glRenderer::Win32glRenderer(Win32ApplicationWindow* _win32window)
		: win32window			{ _win32window }
		, viewProjectionMatrix	{ IDENTITY4 }
	{
		hdc = ::GetDC(win32window->hwnd);
		AL_ASSERT_MSG_NO_DISCARD(hdc, "Win32 :: OpenGL :: Unable to retrieve device context")

		PIXELFORMATDESCRIPTOR pfd =
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

		int pixelFormat = ::ChoosePixelFormat(hdc, &pfd);
		AL_ASSERT_MSG_NO_DISCARD(pixelFormat != 0, "Win32 :: OpenGL :: Unable to choose pixel format for a given descriptor")

		bool isPixelFormatSet = ::SetPixelFormat(hdc, pixelFormat, &pfd);
		AL_ASSERT_MSG_NO_DISCARD(isPixelFormatSet, "Win32 :: OpenGL :: Unable to set pixel format")

		HGLRC renderContext = ::wglCreateContext(hdc);
		AL_ASSERT_MSG_NO_DISCARD(renderContext, "Win32 :: OpenGL :: Unable to create openGL context")

		win32window->hglrc = renderContext;

		bool isCurrent = ::wglMakeCurrent(hdc, renderContext);
		AL_ASSERT_MSG_NO_DISCARD(isCurrent, "Win32 :: OpenGL :: Unable to make openGL context current")

		GLenum glewInitRes = glewInit();
		AL_ASSERT_MSG_NO_DISCARD(glewInitRes == GLEW_OK, "Win32 :: OpenGL :: Unable to init GLEW")
	}

	Win32glRenderer::~Win32glRenderer()
	{
		::wglMakeCurrent(hdc, NULL);
		::wglDeleteContext(win32window->hglrc);

		::ReleaseDC(win32window->hwnd, hdc);
	}

	void Win32glRenderer::make_current()
	{
		::wglMakeCurrent(hdc, win32window->hglrc);
		::glEnable(GL_DEPTH_TEST);
		::glDepthFunc(GL_LESS);
	}

	void Win32glRenderer::set_view_projection(const float4x4& vp)
	{
		viewProjectionMatrix = vp;
	}

	void Win32glRenderer::clear_screen(const float3& color)
	{
		using namespace al::elements;
		::glClearColor(color[R], color[G], color[B], 1.0f);
		::glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void Win32glRenderer::draw(const Shader* shader, const VertexArray* va, const float4x4& trf)
	{
		va->bind();
		shader->bind();

		// OpenGL stores matrices in column-major order, so we need to transpose our matrix
		shader->set_mat4(MODEL_MATRIX_NAME, trf.transpose());
		shader->set_mat4(VP_MATRIX_NAME, viewProjectionMatrix.transpose());
		::glDrawElements(GL_TRIANGLES, va->get_index_buffer()->get_count(), GL_UNSIGNED_INT, nullptr);
	}

	void Win32glRenderer::commit()
	{
		::SwapBuffers(hdc);
	}
}
