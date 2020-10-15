#ifndef __ALFINA_WINDOWS_OPENGL_VERTEX_ARRAY_H__
#define __ALFINA_WINDOWS_OPENGL_VERTEX_ARRAY_H__

#include "../base_vertex_array.h"

namespace al::engine
{
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
}

#include "windows_opengl_vertex_array.cpp"

#endif
