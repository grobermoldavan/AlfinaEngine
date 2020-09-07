#ifndef __ALFINA_WINDOWS_OPENGL_RENDERER_H__
#define __ALFINA_WINDOWS_OPENGL_RENDERER_H__

#include <windows.h>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GL/GL.h>

#include "engine/renderer/base_renderer.h"
#include "engine/math/math.h"

#include "windows_opengl_shader.h"
#include "windows_opengl_vertex_buffer.h"
#include "windows_opengl_index_buffer.h"
#include "windows_opengl_vertex_array.h"

namespace al::engine
{
	class Win32ApplicationWindow;

	class Win32glRenderer : public Renderer
	{
	public:
		Win32glRenderer(Win32ApplicationWindow* _win32window);
		~Win32glRenderer();

		virtual void make_current		()																	override;
		virtual void set_view_projection(const float4x4& vp)												override;
		virtual void clear_screen		(const float3& color)												override;
		virtual void draw				(const Shader* shader, const VertexArray* va, const float4x4& trf)	override;
		virtual void set_vsync			(const bool isEnabled)												override;
		virtual void commit				()																	override;

	private:
		Win32ApplicationWindow* win32window;
		HDC                     hdc;

		float4x4				viewProjectionMatrix;

		typedef BOOL(APIENTRY *PFNWGLSWAPINTERVALPROC)(int);
		PFNWGLSWAPINTERVALPROC wglSwapIntervalEXT = nullptr;
	};
}

#if defined(AL_UNITY_BUILD)
#	include "windows_opengl_renderer.cpp"
#else

#endif

#endif
