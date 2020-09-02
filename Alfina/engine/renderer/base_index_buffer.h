#ifndef __ALFINA_BASE_INDEX_BUFFER_H__
#define __ALFINA_BASE_INDEX_BUFFER_H__

#include <cstdint>

#include "engine/engine_utilities/error_info.h"

namespace al::engine
{
	class IndexBuffer
	{
	public:
		virtual ~IndexBuffer() = default;

		virtual         void            bind        ()  const = 0;
		virtual         void            unbind      ()  const = 0;
		virtual const   uint32_t        get_count   ()  const = 0;
	};

	extern ErrorInfo create_index_buffer	(IndexBuffer**, uint32_t*, uint32_t);
	extern ErrorInfo destroy_index_buffer   (const IndexBuffer*);
}

#endif