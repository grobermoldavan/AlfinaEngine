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

	libraries

	sounds
	ecs
	*/

	AL_LOG(al::engine::Logger::WARNING, "Test warning ", 1, 2.0f, 3.1234, al::float2{ 2, 3 })
	
	al::engine::ApplicationWindow* window;
	al::engine::WindowProperties properties
	{
		al::nonSpecifiedValue,		// size x
		al::nonSpecifiedValue,		// size y
		al::engine::WindowProperties::ScreenMode::WINDOWED,
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
		{ 
			{ 0, 10, -20 },	//position 
			{ 0, 0, 0},		//rotation
			{ 1, 1, 1}		//scale
		},
		{ window->properties.width, window->properties.height },
		0.1f,
		10000.0f,
		90
	};

	al::engine::Geometry geometry;
	al::engine::load_geometry_obj(&geometry, "Assets\\deer.obj");

	int mousePos[2] = { 0, 0 };

	while (true)
	{
		//TestTimer timer;

		{ // process input
			al::engine::get_window_inputs(window, &inputBuffer);

			// closing window
			if (inputBuffer.generalInput.get_flag(al::engine::ApplicationWindowInput::GeneralInputFlags::CLOSE_BUTTON_PRESSED)) break;
			if (inputBuffer.keyboard.buttons.get_flag(al::engine::ApplicationWindowInput::KeyboardInputFlags::ESCAPE)) break;

			// camera movement
			al::engine::Transform& cameraTransform = camera.get_transform();
			al::float3 cameraPosition = cameraTransform.get_position();

			if (inputBuffer.keyboard.buttons.get_flag(al::engine::ApplicationWindowInput::KeyboardInputFlags::W))
			{
				cameraPosition = cameraPosition.add(cameraTransform.get_forward());
			}
			if (inputBuffer.keyboard.buttons.get_flag(al::engine::ApplicationWindowInput::KeyboardInputFlags::S))
			{
				cameraPosition = cameraPosition.add(cameraTransform.get_forward() * -1.0f);
			}
			if (inputBuffer.keyboard.buttons.get_flag(al::engine::ApplicationWindowInput::KeyboardInputFlags::A))
			{
				cameraPosition = cameraPosition.add(cameraTransform.get_right() * -1.0f);
			}
			if (inputBuffer.keyboard.buttons.get_flag(al::engine::ApplicationWindowInput::KeyboardInputFlags::D))
			{
				cameraPosition = cameraPosition.add(cameraTransform.get_right());
			}

			cameraTransform.set_position(cameraPosition);

			// camera rotation
			if (inputBuffer.mouse.buttons.get_flag(al::engine::ApplicationWindowInput::MouseInputFlags::RMB_PRESSED))
			{
				al::float3 angles = cameraTransform.get_rotation();

				int mouseDelta[2] = 
				{
					inputBuffer.mouse.x - mousePos[0],
					inputBuffer.mouse.y - mousePos[1]
				};

				angles[0] += (float)mouseDelta[1] * 0.1f; if (angles[0] > 90.0f || al::is_equal(angles[0], 90.0f)) angles[0] = 90.0f; else if (angles[0] < -90.0f || al::is_equal(angles[0], -90.0f)) angles[0] = -90.0f;
				angles[1] += (float)mouseDelta[0] * 0.1f;
				angles[2] = 0;

				cameraTransform.set_rotation(angles);
			}

			mousePos[0] = inputBuffer.mouse.x;
			mousePos[1] = inputBuffer.mouse.y;
		}

		al::engine::Transform trf{
			{ 0, 0, 0 },
			{ 0, 0, 0 },
			{ 0.01f, 0.01f, 0.01f }
		};

		window->renderer->make_current();
		window->renderer->set_view_projection(camera.get_projection() * camera.get_view());

		window->renderer->clear_screen({ 0.1f, 0.1f, 0.1f });
		shader->set_float4("tint", { 1.0f, 1.0f, 1.0f, 1.0f });
		window->renderer->draw(shader, geometry.va, trf.get_matrix());

		window->renderer->commit();
	}

	al::engine::destroy_shader(shader);

	al::engine::destroy_application_window(window);

    return 0;
}
