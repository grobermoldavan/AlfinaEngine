#ifndef __ALFINA_BASE_RENDERER_H__
#define __ALFINA_BASE_RENDERER_H__

#include "engine/engine_utilities/error_info.h"

//#include "base_shader.h"
//#include "base_vertex_buffer.h"
//#include "base_index_buffer.h"
//#include "base_vertex_array.h"

namespace al::engine
{
	class Renderer
	{
	public:
		virtual ~Renderer() = default;

		virtual void commit	() = 0;
		virtual void wait	() = 0;
	};

	class ApplicationWindow;

	ErrorInfo	create_renderer	(Renderer** renderer, ApplicationWindow* window, std::function<uint8_t*(size_t sizeBytes)> allocate);
	ErrorInfo	destroy_renderer(Renderer* renderer, std::function<void(uint8_t* ptr)> deallocate);
}

#endif
