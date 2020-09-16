#if defined(AL_UNITY_BUILD)

#else
#	include "geometry.h"
#endif

#include <cstdint>
#include <functional>
#include <vector>
#include <stdlib.h>

#include "engine/platform/base_file_sys.h"

#include "engine/engine_utilities/string/string_utilities.h"

namespace al::engine
{
	Geometry::Geometry()
		: vb{ nullptr }
		, ib{ nullptr }
		, va{ nullptr }
	{ }

	Geometry::Geometry(SourceType type, const char* path)
		: vb{ nullptr }
		, ib{ nullptr }
		, va{ nullptr }
	{
		switch (type)
		{
			case SourceType::OBJ: 
			{ 
				load_geometry_obj(this, path); 
				break; 
			}
			default: 
			{ 
				AL_LOG(al::engine::Logger::ERROR_MSG, "Unknown geometry source type.") 
			}
		}
	}


	Geometry::~Geometry()
	{
		destroy_vertex_buffer(vb);
		destroy_index_buffer(ib);
		destroy_vertex_array(va);
	}

	ErrorInfo load_geometry_obj(Geometry* geometry, const char* fileName)
	{
		AL_ASSERT(!geometry->vb);
		AL_ASSERT(!geometry->ib);
		AL_ASSERT(!geometry->va);

		FileHandle file;
		ErrorInfo result = FileSys::read_file(fileName, &file);
		if (result)
		{
			// @TODO : move vert struct to header file
			struct vert
			{
				float pos[3];
				float norm[3] = { 0, 0, 0 };
			};

			std::vector<vert> v;
			std::vector<uint32_t> f;

			const char* ptr = reinterpret_cast<const char*>(file.get_data());

			for_each_line(ptr, file.get_size(), [&](const char* const line, const size_t lineLength)
			{
				if (is_starts_with(line, "v ")) 
				{ 
					const char* ptr = line + 1;
					v.push_back(vert());
					for (int i = 0; i < 3; ++i)
					{
						size_t len;
						get_next_word(ptr, lineLength - (ptr - line), &ptr, &len);
						v[v.size() - 1].pos[i] = std::atof(ptr);
						ptr += len;
					}
				}
				else if (is_starts_with(line, "f "))
				{
					const char* ptr = line + 1;
					for (int i = 0; i < 3; ++i)
					{
						size_t len;
						get_next_word(ptr, lineLength - (ptr - line), &ptr, &len);
						f.push_back(static_cast<uint32_t>(std::atoi(ptr)) - 1);
						ptr += len;
					}
				}
				// @TODO : add other types
			});

			{ // calc normals
				for (size_t it = 0; it < f.size(); it += 3)
				{
					#define TR(name, vt, id) name[id] = v[f[it + vt]].pos[id] - v[f[it]].pos[id]
					float _u[3]; TR(_u, 1, 0); TR(_u, 1, 1); TR(_u, 1, 2);
					float _v[3]; TR(_v, 2, 0); TR(_v, 2, 1); TR(_v, 2, 2);

					float normal[3];
					normal[0] = _u[1] * _v[2] - _u[2] * _v[1];
					normal[1] = _u[2] * _v[0] - _u[0] * _v[2];
					normal[2] = _u[0] * _v[1] - _u[1] * _v[0];

					v[f[it + 0]].norm[0] += normal[0];
					v[f[it + 0]].norm[1] += normal[1];
					v[f[it + 0]].norm[2] += normal[2];

					v[f[it + 1]].norm[0] += normal[0];
					v[f[it + 1]].norm[1] += normal[1];
					v[f[it + 1]].norm[2] += normal[2];

					v[f[it + 2]].norm[0] += normal[0];
					v[f[it + 2]].norm[1] += normal[1];
					v[f[it + 2]].norm[2] += normal[2];
				}
			}

			create_vertex_buffer(&geometry->vb, v.data(), v.size() * sizeof(vert));
			geometry->vb->set_layout
			({
				{ al::engine::ShaderDataType::Float3 },
				{ al::engine::ShaderDataType::Float3 }
			});
			
			create_index_buffer(&geometry->ib, f.data(), f.size());

			create_vertex_array(&geometry->va);
			geometry->va->set_vertex_buffer(geometry->vb);
			geometry->va->set_index_buffer(geometry->ib);

			return{ al::engine::ErrorInfo::Code::ALL_FINE };
		}
		else
		{
			return result;
		}
	}
}
