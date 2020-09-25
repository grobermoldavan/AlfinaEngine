#if defined(AL_UNITY_BUILD)

#else
#	include "windows_opengl_vertex_array.h"
#endif

#include "engine/allocation/allocation.h"

namespace al::engine
{
	ErrorInfo create_vertex_array(VertexArray** va)
	{
		*va = static_cast<VertexArray*>(AL_DEFAULT_CONSTRUCT(Win32glVertexArray, "VERTEX_ARRAY"));
		if (*va)
		{
			return{ ErrorInfo::Code::ALL_FINE };
		}
		else
		{
			return{ ErrorInfo::Code::BAD_ALLOC };
		}
	}

	ErrorInfo destroy_vertex_array(const VertexArray* va)
	{
		if (va) AL_DEFAULT_DESTRUCT(const_cast<VertexArray*>(va), "VERTEX_ARRAY");
		return{ ErrorInfo::Code::ALL_FINE };
	}

	Win32glVertexArray::Win32glVertexArray()
		: vertexBufferIndex{ 0 }
		, vb{ nullptr }
		, ib{ nullptr }
	{
		::glCreateVertexArrays(1, &rendererId);
	}

	Win32glVertexArray::~Win32glVertexArray()
	{
		::glDeleteVertexArrays(1, &rendererId);
	}

	ErrorInfo Win32glVertexArray::set_vertex_buffer(const VertexBuffer* vertexBuffer)
	{
		static const GLenum ShaderDataTypeToOpenGlBaseType[] =
		{
			GL_FLOAT, GL_FLOAT, GL_FLOAT, GL_FLOAT, // floats
			GL_INT  , GL_INT  , GL_INT  , GL_INT,   // ints
			GL_FLOAT, GL_FLOAT,                     // mats
			GL_BOOL                                 // bool
		};

		::glBindVertexArray(rendererId);
		vb = vertexBuffer;
		vb->bind();

		vertexBufferIndex = 0;

		const auto& layout = vb->get_layout();
		for (const auto& element : layout)
		{
			switch (element.type)
			{
			case ShaderDataType::Float:
			case ShaderDataType::Float2:
			case ShaderDataType::Float3:
			case ShaderDataType::Float4:
			case ShaderDataType::Int:
			case ShaderDataType::Int2:
			case ShaderDataType::Int3:
			case ShaderDataType::Int4:
			case ShaderDataType::Bool:
			{
				::glEnableVertexAttribArray(vertexBufferIndex);
				::glVertexAttribPointer
				(
					vertexBufferIndex,
					ShaderDataTypeComponentCount[element.type],
					ShaderDataTypeToOpenGlBaseType[element.type],
					element.isNormalized ? GL_TRUE : GL_FALSE,
					layout.get_stride(),
					reinterpret_cast<const void*>(element.offset)
				);

				vertexBufferIndex++;
				break;
			}
			case ShaderDataType::Mat3:
			case ShaderDataType::Mat4:
			{
				uint8_t count = ShaderDataTypeComponentCount[element.type];
				for (uint8_t it = 0; it < count; ++it)
				{
					::glEnableVertexAttribArray(vertexBufferIndex);
					::glVertexAttribPointer
					(
						vertexBufferIndex,
						count,
						ShaderDataTypeToOpenGlBaseType[element.type],
						element.isNormalized ? GL_TRUE : GL_FALSE,
						layout.get_stride(),
						reinterpret_cast<const void*>(element.offset + sizeof(float) * count * it)
					);

					::glVertexAttribDivisor(vertexBufferIndex, 1);
					++vertexBufferIndex;
				}
				break;
			}
			default:
				AL_ASSERT_MSG(false, "Unknown ShaderDataType")
				return{ ErrorInfo::Code::INCORRECT_INPUT_DATA };
			}
		}

		return{ ErrorInfo::Code::ALL_FINE };
	}

	void Win32glVertexArray::set_index_buffer(const IndexBuffer* indexBuffer)
	{
		::glBindVertexArray(rendererId);
		ib = indexBuffer;
		ib->bind();
	}
}
