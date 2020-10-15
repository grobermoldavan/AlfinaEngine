#ifndef __ALFINA_WINDOWS_OPENGL_RENDERER_H__
#define __ALFINA_WINDOWS_OPENGL_RENDERER_H__

#include <windows.h>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GL/wglew.h>
#include <GL/GL.h>

#include "engine/renderer/base_renderer.h"

//#include "windows_opengl_shader.h"
//#include "windows_opengl_vertex_buffer.h"
//#include "windows_opengl_index_buffer.h"
//#include "windows_opengl_vertex_array.h"

#include "engine/window/win32/windows_application_window.h"
#include "engine/memory/stack_allocator.h"
#include "utilities/flags.h"

namespace al::engine
{
	class Win32ApplicationWindow;

	struct Win32RendererState
	{
		enum StateFlags
		{
			IS_DESTRUCTING
		};

		Flags32 flags;
	};

	struct RenderThreadArg
	{
		Win32ApplicationWindow* win32window;
		HANDLE					initEvent;		// Event to notify main thread that initialization of render thread is done
		HANDLE					kickOffEvent;	// Look down
		HANDLE					finishEvent;	// Look down
		Win32RendererState*		state;
	};

	class Win32glRenderer : public Renderer
	{
	public:
		Win32glRenderer(ApplicationWindow* window, StackAllocator* allocator);
		~Win32glRenderer();

		virtual void commit	() override;
		virtual void wait	() override;

	private:
		Win32RendererState		state;

		// There is two command queues - one is filling up from the main thread and the other gets processed in the render thread
		StackAllocator			commandQueues[2];

		RenderThreadArg			threadArg;
		HANDLE					renderThread;
		HANDLE					kickOffEvent;	// Event to notify render thread that it's must start processing next frame
		HANDLE					finishEvent;	// Event to notify main thread that rendering of frame is finished
	};

	DWORD render_update(LPVOID voidArg);
}

#include "windows_opengl_renderer.cpp"

#endif
