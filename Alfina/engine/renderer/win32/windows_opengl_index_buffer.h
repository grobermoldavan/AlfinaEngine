#ifndef __ALFINA_WINDOWS_OPENGL_INDEX_BUFFER_H__
#define __ALFINA_WINDOWS_OPENGL_INDEX_BUFFER_H__

#include "../base_index_buffer.h"

namespace al::engine
{
	class Win32glIndexBuffer : public IndexBuffer
	{
	public:
		Win32glIndexBuffer(uint32_t* indices, uint32_t _count);
		~Win32glIndexBuffer();

		virtual			void		bind()		const override { ::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rendererId); }
		virtual			void		unbind()	const override { ::glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); }
		virtual const	uint32_t	get_count() const override { return count; }

	private:
		uint32_t    rendererId;
		uint32_t    count;
	};
}

#if defined(AL_UNITY_BUILD)
#	include "windows_opengl_index_buffer.cpp"
#else

#endif

#endif
