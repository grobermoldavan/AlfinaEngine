#if defined(AL_UNITY_BUILD)

#else
#	include "windows_opengl_renderer.h"
#endif

#include <vector>

#include "engine/engine_utilities/asserts.h"

namespace al::engine
{
	// ======================================
	// Shader creation and destruction
	// ======================================

	ErrorInfo create_shader(Shader** shader, const char* vertexShaderSrc, const char* fragmentShaderSrc)
	{
		*shader = static_cast<Shader*>(new Win32glShader(vertexShaderSrc, fragmentShaderSrc));
		return{ ErrorInfo::Code::ALL_FINE };
	}

	ErrorInfo destroy_shader(const Shader* shader)
	{
		if (shader) delete shader;
		return{ ErrorInfo::Code::ALL_FINE };
	}

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
		if (vb) delete vb;
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
		if (ib) delete ib;
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
		if (va) delete va;
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
		if (renderer) delete renderer;
		return{ ErrorInfo::Code::ALL_FINE };
	}

	// ======================================
	// Shader
	// ======================================

	GLenum Win32glShader::SHADER_TYPE_TO_GL_ENUM[] =
	{
		GL_VERTEX_SHADER,
		GL_FRAGMENT_SHADER
	};

	Win32glShader::Win32glShader(const char* vertexShaderSrc, const char* fragmentShaderSrc)
	{
		sources[ShaderType::VERTEX] = vertexShaderSrc;
		sources[ShaderType::FRAGMENT] = fragmentShaderSrc;
		Compile();
	}

	Win32glShader::~Win32glShader()
	{
		::glDeleteProgram(rendererId);
	}

	void Win32glShader::Compile()
	{
		rendererId = ::glCreateProgram();
		AL_ASSERT_MSG_NO_DISCARD(rendererId != 0, "Unable to create OpenGL program via glCreateProgram")

		al::Dispensable<GLenum> shaderIds[ShaderType::__size];

		for (size_t it = 0; it < ShaderType::__size; ++it)
		{
			if (!sources[it].is_specified()) continue;

			ShaderType type = static_cast<ShaderType>(it);
			GLuint shaderId = ::glCreateShader(SHADER_TYPE_TO_GL_ENUM[type]);

			const GLchar* source = sources[it];
			::glShaderSource(shaderId, 1, &source, 0);

			::glCompileShader(shaderId);

			GLint isCompiled = 0;
			::glGetShaderiv(shaderId, GL_COMPILE_STATUS, &isCompiled);
			if (isCompiled != GL_TRUE) HandleShaderCompileError(shaderId);

			::glAttachShader(rendererId, shaderId);
			shaderIds[it] = shaderId;
		}
		
		::glLinkProgram(rendererId);

		GLint isLinked = 0;
		::glGetProgramiv(rendererId, GL_LINK_STATUS, &isLinked);
		if (isLinked != GL_TRUE) HandleProgramCompileError(rendererId, shaderIds);

		for (auto shaderId : shaderIds)
		{
			if (!shaderId.is_specified()) continue;

			::glDetachShader(rendererId, shaderId);
			::glDeleteShader(shaderId);
		}
	}

	void Win32glShader::HandleShaderCompileError(GLuint shaderId)
	{
		GLint infoLength = 0;
		::glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &infoLength);

		std::vector<GLchar> infoLog(infoLength);
		::glGetShaderInfoLog(shaderId, infoLength, &infoLength, &infoLog[0]);

		::glDeleteShader(shaderId);

		AL_LOG_NO_DISCARD(Logger::Type::ERROR_MSG, &infoLog[0])
	}

	void Win32glShader::HandleProgramCompileError(GLuint programId, al::Dispensable<GLenum>(&shaderIds)[ShaderType::__size])
	{
		GLint infoLength = 0;
		::glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &infoLength);

		std::vector<GLchar> infoLog(infoLength);
		::glGetProgramInfoLog(programId, infoLength, &infoLength, &infoLog[0]);

		::glDeleteProgram(programId);

		for (auto shaderId : shaderIds)
		{
			if (shaderId.is_specified())
				::glDeleteShader(shaderId);
		}

		AL_LOG_NO_DISCARD(Logger::Type::ERROR_MSG, &infoLog[0])
	}

	void Win32glShader::set_int(const char* name, const int value) const
	{
		GLint location = ::glGetUniformLocation(rendererId, name);
		AL_ASSERT_MSG_NO_DISCARD(location != -1, "Unable to find uniform location. Variable name is ", name)
		::glUniform1i(location, value);
	}

	void Win32glShader::set_int2(const char* name, const al::int32_2& value) const
	{
		GLint location = ::glGetUniformLocation(rendererId, name);
		AL_ASSERT_MSG_NO_DISCARD(location != -1, "Unable to find uniform location. Variable name is ", name)
		::glUniform2i(location, value[0], value[1]);
	}

	void Win32glShader::set_int3(const char* name, const al::int32_3& value) const
	{
		GLint location = ::glGetUniformLocation(rendererId, name);
		AL_ASSERT_MSG_NO_DISCARD(location != -1, "Unable to find uniform location. Variable name is ", name)
		::glUniform3i(location, value[0], value[1], value[2]);
	}

	void Win32glShader::set_int4(const char* name, const al::int32_4& value) const
	{
		GLint location = ::glGetUniformLocation(rendererId, name);
		AL_ASSERT_MSG_NO_DISCARD(location != -1, "Unable to find uniform location. Variable name is ", name)
		::glUniform4i(location, value[0], value[1], value[2], value[3]);
	}

	void Win32glShader::set_int_array(const char* name, const int* values, uint32_t count) const
	{
		GLint location = ::glGetUniformLocation(rendererId, name);
		AL_ASSERT_MSG_NO_DISCARD(location != -1, "Unable to find uniform location. Variable name is ", name)
		::glUniform1iv(location, count, values);
	}

	void Win32glShader::set_float(const char* name, const float value) const
	{
		GLint location = ::glGetUniformLocation(rendererId, name);
		AL_ASSERT_MSG_NO_DISCARD(location != -1, "Unable to find uniform location. Variable name is ", name)
		::glUniform1f(location, value);
	}

	void Win32glShader::set_float2(const char* name, const al::float2& value) const
	{
		GLint location = ::glGetUniformLocation(rendererId, name);
		AL_ASSERT_MSG_NO_DISCARD(location != -1, "Unable to find uniform location. Variable name is ", name)
		::glUniform2f(location, value[0], value[1]);
	}

	void Win32glShader::set_float3(const char* name, const al::float3& value) const
	{
		GLint location = ::glGetUniformLocation(rendererId, name);
		AL_ASSERT_MSG_NO_DISCARD(location != -1, "Unable to find uniform location. Variable name is ", name)
		::glUniform3f(location, value[0], value[1], value[2]);
	}

	void Win32glShader::set_float4(const char* name, const al::float4& value) const
	{
		GLint location = ::glGetUniformLocation(rendererId, name);
		AL_ASSERT_MSG_NO_DISCARD(location != -1, "Unable to find uniform location. Variable name is ", name)
		::glUniform4f(location, value[0], value[1], value[2], value[3]);
	}

	void Win32glShader::set_float_array(const char* name, const float* values, uint32_t count) const
	{
		GLint location = ::glGetUniformLocation(rendererId, name);
		AL_ASSERT_MSG_NO_DISCARD(location != -1, "Unable to find uniform location. Variable name is ", name)
		::glUniform1fv(location, count, values);
	}

	void Win32glShader::set_mat3(const char* name, const al::float3x3& value) const
	{
		GLint location = ::glGetUniformLocation(rendererId, name);
		AL_ASSERT_MSG_NO_DISCARD(location != -1, "Unable to find uniform location. Variable name is ", name)
		::glUniformMatrix3fv(location, 1, GL_FALSE, value.data_ptr());
	}

	void Win32glShader::set_mat4(const char* name, const al::float4x4& value) const
	{
		GLint location = ::glGetUniformLocation(rendererId, name);
		AL_ASSERT_MSG_NO_DISCARD(location != -1, "Unable to find uniform location. Variable name is ", name)
		::glUniformMatrix4fv(location, 1, GL_FALSE, value.data_ptr());
	}

	// ======================================
	// Vertex buffer
	// ======================================

	Win32glVertexBuffer::Win32glVertexBuffer(float* vertices, uint32_t size)
		: layout{}
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
		: win32window			{ _win32window }
		, viewProjectionMatrix	{ IDENTITY4 }
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
		::glEnable(GL_DEPTH_TEST);
		::glDepthFunc(GL_LESS);
	}

	void Win32glRenderer::set_view_projection(const float4x4& vp)
	{
		viewProjectionMatrix = vp;
	}

	void Win32glRenderer::clear_screen(const float3& color)
	{
		using namespace al::elements;
		::glClearColor(color[R], color[G], color[B], 1.0f);
		::glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void Win32glRenderer::draw(const Shader* shader, const VertexArray* va, const float4x4& trf)
	{
		va->bind();
		shader->bind();

		// OpenGL stores matrices in column-major order, so we need to transpose our matrix
		shader->set_mat4(MODEL_MATRIX_NAME, trf.transpose());
		shader->set_mat4(VP_MATRIX_NAME, viewProjectionMatrix.transpose());
		::glDrawElements(GL_TRIANGLES, va->get_index_buffer()->get_count(), GL_UNSIGNED_INT, nullptr);
	}

	void Win32glRenderer::commit()
	{
		::SwapBuffers(hdc);
	}
}