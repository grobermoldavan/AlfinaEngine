#if defined(AL_UNITY_BUILD)

#else
#	include "windows_opengl_vertex_buffer.h"
#endif

namespace al::engine
{
	ErrorInfo create_vertex_buffer(VertexBuffer** vb, const void* vertices, uint32_t size)
	{
		*vb = static_cast<VertexBuffer*>(new Win32glVertexBuffer(vertices, size));
		return{ ErrorInfo::Code::ALL_FINE };
	}

	ErrorInfo destroy_vertex_buffer(const VertexBuffer* vb)
	{
		if (vb) delete vb;
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
