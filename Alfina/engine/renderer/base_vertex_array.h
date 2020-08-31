#ifndef __ALFINA_BASE_VERTEX_ARRAY_H__
#define __ALFINA_BASE_VERTEX_ARRAY_H__

#include "engine/engine_utilities/error_info.h"

namespace al::engine
{
	class VertexBuffer;
	class IndexBuffer;

	class VertexArray
	{
	public:
		virtual ~VertexArray() = default;

		virtual void bind   () const = 0;
		virtual void unbind () const = 0;

		virtual ErrorInfo   set_vertex_buffer(const VertexBuffer*    vertexBuffer) = 0;
		virtual void        set_index_buffer (const IndexBuffer*     indexBuffer)  = 0;

		virtual const VertexBuffer* get_vertex_buffer() const = 0;
		virtual const IndexBuffer*  get_index_buffer () const = 0;
	};

	extern ErrorInfo create_vertex_array    (VertexArray**);
	extern ErrorInfo destroy_vertex_array   (const VertexArray*);
}

#endif