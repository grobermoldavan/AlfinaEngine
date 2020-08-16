#include "project.h"

int main()
{
	AL_LOG(al::engine::Logger::WARNING, "Test warning")

	al::engine::ApplicationWindow* window;
	al::engine::WindowProperties properties
	{
		al::nonSpecifiedValue,
		al::nonSpecifiedValue,
		al::nonSpecifiedValue,
		al::nonSpecifiedValue,
		al::engine::WindowProperties::ScreenMode::WINDOWED,
		"Application window"
	};
	al::engine::create_application_window(properties, &window);

	while (!window->input.generalInput.get_flag(al::engine::ApplicationWindowInput::GeneralInputFlags::CLOSE_BUTTON_PRESSED))
	{
		window->renderer->make_current();
		window->renderer->clear_screen({ 0.1f, 0.1f, 0.1f });

		// setup vertex buffer
		float arr[12] =
		{
			-1.0f, -1.0f, 0.0f,
			1.0f, -1.0f, 0.0f,
			-1.0f,  1.0f, 0.0f,
			1.0f,  1.0f, 0.0f
		};

		al::engine::VertexBuffer* vb;
		al::engine::create_vertex_buffer(&vb, arr, sizeof(float) * 12);
		vb->set_layout
		({
			{ al::engine::ShaderDataType::Float3 },
		});

		// setup index buffer
		uint32_t ids[3] = { 0, 1, 2 };
		al::engine::IndexBuffer* ib;
		al::engine::create_index_buffer(&ib, ids, 3);

		// setup vertex array
		al::engine::VertexArray* va;
		al::engine::create_vertex_array(&va);
		va->set_vertex_buffer(vb);
		va->set_index_buffer(ib);

		// draw vertex array
		va->bind();
		::glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, nullptr);

		al::engine::destroy_vertex_buffer(vb);
		al::engine::destroy_index_buffer(ib);
		al::engine::destroy_vertex_array(va);

		window->renderer->commit();
	}

	al::engine::destroy_application_window(window);

    return 0;
}