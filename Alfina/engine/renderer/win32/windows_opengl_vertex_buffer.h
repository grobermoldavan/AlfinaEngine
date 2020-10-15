#ifndef __ALFINA_WINDOWS_OPENGL_VERTEX_BUFFER_H__
#define __ALFINA_WINDOWS_OPENGL_VERTEX_BUFFER_H__

#include "../base_vertex_buffer.h"

namespace al::engine
{
	class Win32glVertexBuffer : public VertexBuffer
	{
	public:
		Win32glVertexBuffer(const void* data, uint32_t size);
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
}

#include "windows_opengl_vertex_buffer.cpp"

#endif
