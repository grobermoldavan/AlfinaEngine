#if defined(AL_UNITY_BUILD)

#else
#	include "windows_opengl_vertex_buffer.h"
#endif

#include "engine/allocation/allocation.h"

namespace al::engine
{
	ErrorInfo create_vertex_buffer(VertexBuffer** vb, const void* vertices, uint32_t size)
	{
		*vb = static_cast<VertexBuffer*>(AL_DEFAULT_CONSTRUCT(Win32glVertexBuffer, "VERTEX_BUFFER", vertices, size));
		if (*vb)
		{
			return{ ErrorInfo::Code::ALL_FINE };
		}
		else
		{
			return{ ErrorInfo::Code::BAD_ALLOC };
		}
	}

	ErrorInfo destroy_vertex_buffer(const VertexBuffer* vb)
	{
		if (vb) AL_DEFAULT_DESTRUCT(const_cast<VertexBuffer*>(vb), "VERTEX_BUFFER");
		return{ ErrorInfo::Code::ALL_FINE };
	}

	Win32glVertexBuffer::Win32glVertexBuffer(const void* data, uint32_t size)
		: layout{}
	{
		::glCreateBuffers(1, &rendererId);
		::glBindBuffer(GL_ARRAY_BUFFER, rendererId);
		::glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
	}

	Win32glVertexBuffer::~Win32glVertexBuffer()
	{
		::glDeleteBuffers(1, &rendererId);
	}

	void Win32glVertexBuffer::set_data(const void* data, uint32_t size)
	{
		::glBindBuffer(GL_ARRAY_BUFFER, rendererId);
		::glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
	}
}
