#if defined(AL_UNITY_BUILD)

#else
#	include "windows_opengl_index_buffer.h"
#endif

#include "engine/allocation/allocation.h"

namespace al::engine
{
	ErrorInfo create_index_buffer(IndexBuffer** ib, uint32_t* indices, uint32_t count)
	{
		*ib = static_cast<IndexBuffer*>(AL_DEFAULT_CONSTRUCT(Win32glIndexBuffer, "INDEX_BUF", indices, count));
		if (*ib)
		{
			return{ ErrorInfo::Code::ALL_FINE };
		}
		else
		{
			return{ ErrorInfo::Code::BAD_ALLOC };
		}
	}

	ErrorInfo destroy_index_buffer(const IndexBuffer* ib)
	{
		if (ib) AL_DEFAULT_DESTRUCT(const_cast<IndexBuffer*>(ib), "INDEX_BUF");
		return{ ErrorInfo::Code::ALL_FINE };
	}

	Win32glIndexBuffer::Win32glIndexBuffer(uint32_t* indices, uint32_t _count)
		: count{ _count }
	{
		::glCreateBuffers(1, &rendererId);

		// hack
		::glBindBuffer(GL_ARRAY_BUFFER, rendererId);
		::glBufferData(GL_ARRAY_BUFFER, _count * sizeof(uint32_t), indices, GL_STATIC_DRAW);
	}

	Win32glIndexBuffer::~Win32glIndexBuffer()
	{
		::glDeleteBuffers(1, &rendererId);
	}
}
