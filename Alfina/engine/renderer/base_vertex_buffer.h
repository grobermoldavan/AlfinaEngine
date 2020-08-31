#ifndef __ALFINA_BASE_VERTEX_BUFFER_H__
#define __ALFINA_BASE_VERTEX_BUFFER_H__

#include <cstdint>
#include <string>

#include "engine/engine_utilities/error_info.h"

namespace al::engine
{
	enum ShaderDataType : uint8_t
	{
		Float, Float2, Float3, Float4,  // floats
		Int, Int2, Int3, Int4,          // ints
		Mat3, Mat4,						// mats
		Bool                            // bool
	};

	static const uint32_t ShaderDataTypeSize[] =
	{
		4        , 4 * 2    , 4 * 3, 4 * 4,     // floats
		4        , 4 * 2    , 4 * 3, 4 * 4,     // ints
		4 * 3 * 3, 4 * 4 * 4,                   // mats
		1                                       // bool
	};

	static const uint32_t ShaderDataTypeComponentCount[] =
	{
		1, 2, 3, 4,     // floats
		1, 2, 3, 4,     // ints
		3, 4,           // mats
		1               // bool
	};

	struct BufferElement
	{
		std::string     name;
		uint32_t        size;
		uint32_t        offset;
		bool            isNormalized;
		ShaderDataType  type;

		BufferElement() = default;

		BufferElement(ShaderDataType _type, const std::string& _name = "", bool _isNormalized = false)
			: name			{ _name }
			, size			{ ShaderDataTypeSize[static_cast<uint8_t>(_type)] }
			, offset		{ 0 }
			, isNormalized	{ _isNormalized }
			, type			{ _type }
		{ }
	};

	class BufferLayout
	{
	public:
		BufferLayout() = default;

		BufferLayout(const std::initializer_list<BufferElement>& _elements)
			: elements{ _elements }
		{
			calculate_offset_and_stride();
		}

		const uint32_t                      get_stride	() const	{ return stride; }
		const std::vector<BufferElement>&   get_elements() const	{ return elements; }

		std::vector<BufferElement>::iterator       begin()			{ return elements.begin(); }
		std::vector<BufferElement>::iterator       end	()			{ return elements.end(); }
		std::vector<BufferElement>::const_iterator begin() const	{ return elements.begin(); }
		std::vector<BufferElement>::const_iterator end	() const	{ return elements.end(); }

	private:
		void calculate_offset_and_stride()
		{
			size_t offset = 0;
			for (auto& element : elements)
			{
				element.offset = offset;
				offset += element.size;
			}
			stride = offset;
		}

		std::vector<BufferElement>  elements;
		uint32_t                    stride;
	};

	class VertexBuffer
	{
	public:
		virtual ~VertexBuffer() = default;

		virtual         void            bind()										const	= 0;
		virtual         void            unbind()									const	= 0;
		virtual         void            set_data(const void* data, uint32_t size)			= 0;
		virtual         void            set_layout(const BufferLayout& layout)				= 0;
		virtual const   BufferLayout&   get_layout()                                const	= 0;
	};

	extern ErrorInfo create_vertex_buffer	(VertexBuffer**, float*, uint32_t);
	extern ErrorInfo destroy_vertex_buffer	(const VertexBuffer*);
}

#endif