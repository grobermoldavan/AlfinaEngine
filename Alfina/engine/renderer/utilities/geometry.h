#ifndef __ALFINA_GEOMETRY_H__
#define __ALFINA_GEOMETRY_H__

#include <vector>

#include "../base_vertex_buffer.h"
#include "../base_index_buffer.h"
#include "../base_vertex_array.h"

#include "engine/engine_utilities/error_info.h"

namespace al::engine
{
	class Geometry
	{
	public:
		enum SourceType
		{
			OBJ,
			__end
		};

		Geometry();
		Geometry(SourceType type, const char* path);
		~Geometry();

	public:
		VertexBuffer* vb;
		IndexBuffer* ib;
		VertexArray* va;
	};

	ErrorInfo load_geometry_obj(Geometry*, const char* fileName);
}

#if defined(AL_UNITY_BUILD)
#	include "geometry.cpp"
#else

#endif

#endif
