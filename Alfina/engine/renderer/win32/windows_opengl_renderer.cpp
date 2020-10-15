
#include "windows_opengl_renderer.h"

#include "engine/window/win32/windows_application_window.h"

namespace al::engine
{
	Win32glRenderer::Win32glRenderer(ApplicationWindow* window, StackAllocator* allocator)
        : Renderer{ window, allocator }
	{
		// Create kick off event
		kickOffEvent = ::CreateEvent(	NULL,					// default security attributes
										TRUE,					// manual-reset event
										FALSE,					// initial state is nonsignaled
										TEXT("renderEvent"));	// object name

		// Create finish event
		finishEvent = ::CreateEvent(	NULL,					// default security attributes
										TRUE,					// manual-reset event
										FALSE,					// initial state is nonsignaled
										TEXT("finishEvent"));	// object name

		// Start the render thread
		threadArg =
		{
			static_cast<Win32ApplicationWindow*>(window),
			::CreateEvent(NULL, TRUE, FALSE, TEXT("InitEvent")),
			kickOffEvent,
			finishEvent,
			&state
		};
		renderThread = ::CreateThread(NULL, 0, render_update, &threadArg, 0, NULL);
		::WaitForSingleObject(threadArg.initEvent, INFINITE);
		::CloseHandle(threadArg.initEvent);

		// Initial kick off
		::SetEvent(kickOffEvent);
	}

	Win32glRenderer::~Win32glRenderer()
	{
		// Set flag
		state.flags.set_flag(Win32RendererState::StateFlags::IS_DESTRUCTING);

		// Last commit
		commit();

		// Wait for thread
		::WaitForSingleObject(renderThread, INFINITE);
		::CloseHandle(renderThread);
		
		// Close handle to an events
		::CloseHandle(finishEvent);
		::CloseHandle(kickOffEvent);
	}

	void Win32glRenderer::commit()
	{
		// Wait for current work to be finished
		wait();

		// Notify render thread
		::ResetEvent(finishEvent);
		::SetEvent(kickOffEvent);
	}

	void Win32glRenderer::wait()
	{
		::WaitForSingleObject(finishEvent, INFINITE);
	}

	DWORD render_update(LPVOID voidArg)
	{
		// @TODO : add error handling

		RenderThreadArg* renderArgPtr = static_cast<RenderThreadArg*>(voidArg);
		Win32ApplicationWindow* win32window = renderArgPtr->win32window;

		HDC deviceContext = ::GetDC(win32window->hwnd);
		//AL_ASSERT_MSG_NO_DISCARD(deviceContext, "Win32 :: OpenGL :: Unable to retrieve device context")

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
		//AL_ASSERT_MSG_NO_DISCARD(pixelFormat != 0, "Win32 :: OpenGL :: Unable to choose pixel format for a given descriptor")

		bool isPixelFormatSet = ::SetPixelFormat(deviceContext, pixelFormat, &pfd);
		//AL_ASSERT_MSG_NO_DISCARD(isPixelFormatSet, "Win32 :: OpenGL :: Unable to set pixel format")

		HGLRC renderContext = ::wglCreateContext(deviceContext);
		//AL_ASSERT_MSG_NO_DISCARD(renderContext, "Win32 :: OpenGL :: Unable to create openGL context")

		win32window->hglrc = renderContext;

		bool isCurrent = ::wglMakeCurrent(deviceContext, renderContext);
		//AL_ASSERT_MSG_NO_DISCARD(isCurrent, "Win32 :: OpenGL :: Unable to make openGL context current")

		GLenum glewInitRes = ::glewInit();
		//AL_ASSERT_MSG_NO_DISCARD(glewInitRes == GLEW_OK, "Win32 :: OpenGL :: Unable to init GLEW")

		::wglMakeCurrent(deviceContext, win32window->hglrc);
		::glEnable(GL_DEPTH_TEST);
		::glDepthFunc(GL_LESS);

		// Vsync
		::wglSwapIntervalEXT(1);

		::SetEvent(renderArgPtr->initEvent);

		// Render loop
		while (!renderArgPtr->state->flags.get_flag(Win32RendererState::StateFlags::IS_DESTRUCTING))
		{
			::WaitForSingleObject(renderArgPtr->kickOffEvent, INFINITE);
			::ResetEvent(renderArgPtr->kickOffEvent);

			// Do stuff here ================================

			static float tint = 0.0f;
			tint += 0.01f;
			if (tint > 1.0f) tint = 0.0f;

			::glClearColor(tint, tint, tint, 1.0f);
			::glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// ==============================================

			::SwapBuffers(deviceContext);

			::SetEvent(renderArgPtr->finishEvent);
		}
		
		::wglMakeCurrent(deviceContext, NULL);
		::wglDeleteContext(win32window->hglrc);

		::ReleaseDC(win32window->hwnd, deviceContext);
		
		return 0;
	}
}
