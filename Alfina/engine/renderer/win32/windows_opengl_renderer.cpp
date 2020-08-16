#if defined(AL_UNITY_BUILD)

#else
#	include "windows_opengl_renderer.h"
#endif

#include "engine/engine_utilities/asserts.h"

namespace al::engine
{
	// ======================================
	// Vertex buffer creation and destruction
	// ======================================

	ErrorInfo create_vertex_buffer(VertexBuffer** vb, float* vertices, uint32_t size)
	{
		*vb = static_cast<VertexBuffer*>(new Win32glVertexBuffer(vertices, size));
		return{ ErrorInfo::Code::ALL_FINE };
	}

	ErrorInfo destroy_vertex_buffer(const VertexBuffer* vb)
	{
		delete vb;
		return{ ErrorInfo::Code::ALL_FINE };
	}

	// ======================================
	// Index buffer creation and destruction
	// ======================================

	ErrorInfo create_index_buffer(IndexBuffer** ib, uint32_t* indices, uint32_t count)
	{
		*ib = static_cast<IndexBuffer*>(new Win32glIndexBuffer(indices, count));
		return{ ErrorInfo::Code::ALL_FINE };
	}

	ErrorInfo destroy_index_buffer(const IndexBuffer* ib)
	{
		delete ib;
		return{ ErrorInfo::Code::ALL_FINE };
	}

	// ======================================
	// Vertex array creation and destruction
	// ======================================

	ErrorInfo create_vertex_array(VertexArray** va)
	{
		*va = static_cast<VertexArray*>(new Win32glVertexArray());
		return{ ErrorInfo::Code::ALL_FINE };
	}

	ErrorInfo destroy_vertex_array(const VertexArray* va)
	{
		delete va;
		return{ ErrorInfo::Code::ALL_FINE };
	}

	// ======================================
	// Renderer creation and destruction
	// ======================================

	ErrorInfo create_renderer(Renderer** renderer, ApplicationWindow* window)
	{
		*renderer = static_cast<Renderer*>(new Win32glRenderer(static_cast<Win32ApplicationWindow*>(window)));
		return{ ErrorInfo::Code::ALL_FINE };
	}

	ErrorInfo destroy_renderer(Renderer* renderer)
	{
		delete renderer;
		return{ ErrorInfo::Code::ALL_FINE };
	}

	// ======================================
	// Vertex buffer
	// ======================================

	Win32glVertexBuffer::Win32glVertexBuffer(float* vertices, uint32_t size)
		: layout{ }
	{
		::glCreateBuffers(1, &rendererId);
		::glBindBuffer(GL_ARRAY_BUFFER, rendererId);
		::glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);
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

	// ======================================
	// Index buffer
	// ======================================

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

	// ======================================
	// Vertex array
	// ======================================

	Win32glVertexArray::Win32glVertexArray()
		: vertexBufferIndex{ 0 }
		, vb { nullptr }
		, ib { nullptr }
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
					(const void*)element.offset
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
						(const void*)(element.offset + sizeof(float) * count * it)
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

	// ======================================
	// Renderer
	// ======================================

	Win32glRenderer::Win32glRenderer(Win32ApplicationWindow* _win32window)
		: win32window{ _win32window }
	{
		hdc = ::GetDC(win32window->hwnd);
		AL_ASSERT_MSG_NO_DISCARD(hdc, "Win32 :: OpenGL :: Unable to retrieve device context")

		PIXELFORMATDESCRIPTOR pfd =
		{
			sizeof(PIXELFORMATDESCRIPTOR),
			1,
			PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, // Flags
			PFD_TYPE_RGBA,                                              // The kind of framebuffer. RGBA or palette.
			32,                                                         // Colordepth of the framebuffer.
			0, 0, 0, 0, 0, 0,
			0,
			0,
			0,
			0, 0, 0, 0,
			24,                                                         // Number of bits for the depthbuffer
			8,                                                          // Number of bits for the stencilbuffer
			0,                                                          // Number of Aux buffers in the framebuffer.
			PFD_MAIN_PLANE,
			0,
			0, 0, 0
		};

		int pixelFormat = ::ChoosePixelFormat(hdc, &pfd);
		AL_ASSERT_MSG_NO_DISCARD(pixelFormat != 0, "Win32 :: OpenGL :: Unable to choose pixel format for a given descriptor")

		bool isPixelFormatSet = ::SetPixelFormat(hdc, pixelFormat, &pfd);
		AL_ASSERT_MSG_NO_DISCARD(isPixelFormatSet, "Win32 :: OpenGL :: Unable to set pixel format")

		HGLRC renderContext = ::wglCreateContext(hdc);
		AL_ASSERT_MSG_NO_DISCARD(renderContext, "Win32 :: OpenGL :: Unable to create openGL context")

		win32window->hglrc = renderContext;

		bool isCurrent = ::wglMakeCurrent(hdc, renderContext);
		AL_ASSERT_MSG_NO_DISCARD(isCurrent, "Win32 :: OpenGL :: Unable to make openGL context current")

		GLenum glewInitRes = glewInit();
		AL_ASSERT_MSG_NO_DISCARD(glewInitRes == GLEW_OK, "Win32 :: OpenGL :: Unable to init GLEW")
	}

	Win32glRenderer::~Win32glRenderer()
	{
		::wglMakeCurrent(hdc, NULL);
		::wglDeleteContext(win32window->hglrc);

		::ReleaseDC(win32window->hwnd, hdc);
	}

	void Win32glRenderer::make_current()
	{
		::wglMakeCurrent(hdc, win32window->hglrc);
	}

	void Win32glRenderer::clear_screen(al::float3 color)
	{
		using namespace al::elements;
		::glClearColor(color[R], color[G], color[B], 1.0f);
		::glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void Win32glRenderer::commit()
	{
		::SwapBuffers(hdc);
	}
}