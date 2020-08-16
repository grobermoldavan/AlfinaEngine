#ifndef __ALFINA_WINDOWS_OPENGL_RENDERER_H__
#define __ALFINA_WINDOWS_OPENGL_RENDERER_H__

#include <windows.h>

#define GLEW_STATIC
#include <GL/glew.h>
#include <GL/GL.h>

#include "engine/renderer/base_renderer.h"
#include "engine/math/math.h"

namespace al::engine
{
	class Win32glVertexBuffer : public VertexBuffer
	{
	public:
		Win32glVertexBuffer(float* vertices, uint32_t size);
		~Win32glVertexBuffer();

		virtual			void			bind		()								const	override { ::glBindBuffer(GL_ARRAY_BUFFER, rendererId); }
		virtual			void			unbind		()								const	override { ::glBindBuffer(GL_ARRAY_BUFFER, 0); }
		virtual			void			set_layout	(const BufferLayout& layout)			override { this->layout = layout; }
		virtual const	BufferLayout&	get_layout	()								const	override { return layout; }

		virtual			void			set_data	(const void* data, uint32_t size) override;

	private:
		BufferLayout    layout;
		uint32_t        rendererId;
	};

	class Win32glIndexBuffer : public IndexBuffer
	{
	public:
		Win32glIndexBuffer(uint32_t* indices, uint32_t _count);
		~Win32glIndexBuffer();

		virtual			void		bind		() const override { ::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rendererId); }
		virtual			void		unbind		() const override { ::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); }
		virtual const	uint32_t	get_count	() const override { return count; }

	private:
		uint32_t    rendererId;
		uint32_t    count;
	};

	class Win32glVertexArray : public VertexArray
	{
	public:
		Win32glVertexArray();
		~Win32glVertexArray();

		virtual			void			bind				() const override { ::glBindVertexArray(rendererId); }
		virtual			void			unbind				() const override { ::glBindVertexArray(0); }

		virtual const	VertexBuffer*	get_vertex_buffer	() const override { return vb; }
		virtual const	IndexBuffer*	get_index_buffer	() const override { return ib; }

		virtual			ErrorInfo		set_vertex_buffer	(const VertexBuffer* vertexBuffer)	override;
		virtual			void			set_index_buffer	(const IndexBuffer* indexBuffer)	override;

	private:
			  uint32_t        rendererId;
			  uint32_t        vertexBufferIndex;
		const VertexBuffer*   vb;
		const IndexBuffer*    ib;
	};

	class Win32ApplicationWindow;

	class Win32glRenderer : public Renderer
	{
	public:
		Win32glRenderer(Win32ApplicationWindow* _win32window);
		~Win32glRenderer();

		virtual void make_current	()					override;
		virtual void clear_screen	(al::float3 color)	override;
		virtual void commit			()					override;

	private:
		Win32ApplicationWindow* win32window;
		HDC                     hdc;
	};
}

#if defined(AL_UNITY_BUILD)
#	include "windows_opengl_renderer.cpp"
#else

#endif // AL_UNITY_BUILD

#endif // __ALFINA_WINDOWS_OPENGL_RENDERER_H__