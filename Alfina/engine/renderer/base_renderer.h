#ifndef __ALFINA_BASE_RENDERER_H__
#define __ALFINA_BASE_RENDERER_H__

#include <cstdint>
#include <cstddef>

#include "engine/engine_utilities/error_info.h"
#include "engine/math/math.h"

#include "shader_constants.h"
#include "utilities/render_camera.h"
#include "utilities/geometry.h"

#include "base_shader.h"
#include "base_vertex_buffer.h"
#include "base_index_buffer.h"
#include "base_vertex_array.h"

namespace al::engine
{
	class Renderer
	{
	public:
		virtual void make_current		()																	= 0;
		virtual void set_view_projection(const float4x4& vp)												= 0;
		virtual void clear_screen		(const float3& color)												= 0;
		virtual void draw				(const Shader* shader, const VertexArray* va, const float4x4& trf)	= 0;
		virtual void set_vsync			(const bool isEnabled)												= 0;
		virtual void commit				()																	= 0;
	};

	class ApplicationWindow;

	extern ErrorInfo create_renderer	(Renderer**, ApplicationWindow*);
	extern ErrorInfo destroy_renderer	(const Renderer*);
}

#endif