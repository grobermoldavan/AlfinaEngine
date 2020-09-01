#include "project.h"

#include <chrono>
#include <type_traits>

int main()
{
	/*
	geometry
	animation
	text rendering
	timing macro
	input events (simple camera controller)
	materials

	sounds
	ecs
	*/

	AL_LOG(al::engine::Logger::WARNING, "Test warning ", 1, 2.0f, 3.1234, al::float2{ 2, 3 })
	
	al::engine::ApplicationWindow* window;
	al::engine::WindowProperties properties
	{
		al::nonSpecifiedValue,		// size x
		al::nonSpecifiedValue,		// size y
		al::engine::WindowProperties::ScreenMode::FULL_SCREEN,
		"Application window"
	};
	al::engine::create_application_window(properties, &window);

	al::engine::FileHandle vertexShader;
	al::engine::FileSys::read_file("Shaders\\vertex.vert", &vertexShader);

	al::engine::FileHandle fragmentShader;
	al::engine::FileSys::read_file("Shaders\\fragment.frag", &fragmentShader);

	al::engine::Shader* shader;
	al::engine::create_shader(&shader, (const char*)vertexShader.get_data(), (const char*)fragmentShader.get_data());

	al::engine::ApplicationWindowInput inputBuffer { };

	class TestTimer
	{
	public:
		TestTimer()
			: start { std::chrono::high_resolution_clock::now() }
		{ }

		~TestTimer()
		{
			auto now = std::chrono::high_resolution_clock::now();
			AL_LOG_SHORT(al::engine::Logger::MESSAGE, "Frame time : ", std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count(), " ms")
		}

	private:
		std::result_of<decltype(&std::chrono::high_resolution_clock::now)()>::type start;
	};

	al::engine::PerspectiveRenderCamera camera
	{
		{ },
		{ window->properties.width, window->properties.height },
		0.1f,
		10000.0f,
		60
	};

	al::engine::Geometry geometry;
	al::engine::load_geometry_obj(&geometry, "Assets\\deer.obj");

	while (true)
	{
		TestTimer timer;

		al::engine::get_window_inputs(window, &inputBuffer);

		if (inputBuffer.generalInput.get_flag(al::engine::ApplicationWindowInput::GeneralInputFlags::CLOSE_BUTTON_PRESSED)) break;
		if (inputBuffer.keyboard.buttons.get_flag(al::engine::ApplicationWindowInput::KeyboardInputFlags::ESCAPE)) break;

		static al::float4 tint{ 1.0f, 1.0f, 1.0f, 1.0f };
		//tint[0] = tint[0] + 0.01f; if (tint[0] > 1.0f) tint[0] -= 1.0f;
		//tint[1] = tint[1] + 0.01f; if (tint[1] > 1.0f) tint[1] -= 1.0f;
		//tint[2] = tint[2] + 0.01f; if (tint[2] > 1.0f) tint[2] -= 1.0f;

		static float rot = 0;
		rot += 0.1f;
		al::engine::Transform trf{
			{ 0, 0, 0 },
			{ 0, 0, 0 },
			{ 0.01f, 0.01f, 0.01f }
		};

		float sine = std::sin(al::to_radians(rot));
		float cosine = std::cos(al::to_radians(rot));
		float radius = 20.0f;

		window->renderer->make_current();
		camera.set_position({ sine * radius, std::sin(rot) + 10, cosine * radius });
		camera.look_at({ 0, 5, 0 }, { 0, 1, 0 });

		window->renderer->set_view_projection(camera.get_projection() * camera.get_view());

		window->renderer->clear_screen({ 0.1f, 0.1f, 0.1f });
		shader->set_float4("tint", tint);
		window->renderer->draw(shader, geometry.va, trf.get_matrix());

		window->renderer->commit();
	}

	al::engine::destroy_shader(shader);

	al::engine::destroy_application_window(window);

    return 0;
}
