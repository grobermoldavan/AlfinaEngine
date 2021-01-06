
#include "alfina_engine_application.h"

#include "engine/rendering/geometry/geometry.h"

#include "utilities/smooth_average.h"

namespace al::engine
{
    void AlfinaEngineApplication::initialize_components() noexcept
    {
        const std::size_t NUM_OF_JOB_THREADS = std::thread::hardware_concurrency() - 2; // Minus two because of the main thread and rendering thread

        MemoryManager* memoryManager = MemoryManager::get();

        debug::globalLogger = memoryManager->get_stack()->allocate_as<debug::Logger>();
        ::new(debug::globalLogger) debug::Logger{ };

        jobSystem = memoryManager->get_stack()->allocate_as<JobSystem>();
        ::new(jobSystem) JobSystem{ NUM_OF_JOB_THREADS };

        fileSystem = memoryManager->get_stack()->allocate_as<FileSystem>();
        ::new(fileSystem) FileSystem{ jobSystem };
        FileSystem::set_global_instance(fileSystem);

        OsWindowParams windowParams;
        windowParams.isFullscreen = false;
        window = create_window(&windowParams);

        renderer = create_renderer<EngineConfig::DEFAULT_RENDERER_TYPE>(window);

        dbgFlyCamera.get_render_camera()->set_aspect_ratio(static_cast<float>(window->get_params()->width) / static_cast<float>(window->get_params()->height));
        renderer->set_camera(dbgFlyCamera.get_render_camera());

        al_log_message(LOG_CATEGORY_BASE_APPLICATION, "Initialized engine components");
    }

    void AlfinaEngineApplication::terminate_components() noexcept
    {
        al_log_message(LOG_CATEGORY_BASE_APPLICATION, "Terminating engine components");

        MemoryManager* memoryManager = MemoryManager::get();

        destroy_renderer<EngineConfig::DEFAULT_RENDERER_TYPE>(renderer);
        destroy_window(window);
        fileSystem->~FileSystem();
        jobSystem->~JobSystem();
        debug::globalLogger->~Logger();
    }

    void AlfinaEngineApplication::run() noexcept
    {
        using ClockT = std::chrono::steady_clock;
        using DtDuration = std::chrono::duration<float>;
        {
            al_profile_scope("Print log buffer");
            debug::globalLogger->print_log_buffer();
        }
        {
            al_profile_scope("Print profile buffer");
            debug::globalLogger->print_profile_buffer();
        }
        al_log_message(LOG_CATEGORY_BASE_APPLICATION, "Starting application");
        frameCount = 0;
        auto previousTime = ClockT::now();
        while(true)
        {
            al_profile_scope("Process frame");
            auto currentTime = ClockT::now();
            auto dt = std::chrono::duration_cast<DtDuration>(currentTime - previousTime).count();
            previousTime = currentTime;
            {
                al_profile_scope("Process window");
                window->process();
                if (window->is_quit())
                {
                    break;
                }
            }
            renderer->start_process_frame();
            update_input();
            simulate(dt);
            renderer->wait_for_command_buffers_toggled();
            dbg_render();
            renderer->wait_for_render_finish();
            process_end_frame();
        }
    }

    void AlfinaEngineApplication::update_input() noexcept
    {
        al_profile_function();

        inputState.toggle();
        OsWindowInput& current = inputState.get_current();
        OsWindowInput& previous = inputState.get_previous();

        current = window->get_input();

        for (std::size_t it = 1; it < static_cast<std::size_t>(OsWindowInput::KeyboardInputFlags::__end); it++)
        {
            bool currentFlag = current.keyboard.buttons.get_flag(it);
            bool previousFlag = previous.keyboard.buttons.get_flag(it);
            if (currentFlag && !previousFlag)
            {
                onKeyboardButtonPressed(static_cast<OsWindowInput::KeyboardInputFlags>(it));
            }
            else if (!currentFlag && previousFlag)
            {
                onKeyboardButtonReleased(static_cast<OsWindowInput::KeyboardInputFlags>(it));
            }
        }

        for (std::size_t it = 0; it < static_cast<std::size_t>(OsWindowInput::MouseInputFlags::__end); it++)
        {
            bool currentFlag = current.mouse.buttons.get_flag(it);
            bool previousFlag = previous.mouse.buttons.get_flag(it);
            if (currentFlag && !previousFlag)
            {
                onMouseButtonPressed(static_cast<OsWindowInput::MouseInputFlags>(it));
            }
            else if (!currentFlag && previousFlag)
            {
                onMouseButtonReleased(static_cast<OsWindowInput::MouseInputFlags>(it));
            }
        }
    }

    void AlfinaEngineApplication::simulate(float dt) noexcept
    {
        static SmoothAverage<float> fps;
        fps.push(dt);
        // al_log_message(LOG_CATEGORY_BASE_APPLICATION, "Fps : %f", 1.0f / fps.get());

        dbgFlyCamera.process_inputs(&inputState.get_current(), dt);
    }

    void AlfinaEngineApplication::dbg_render() noexcept
    {
        al_profile_function();

        static VertexBuffer* vb = nullptr;
        static IndexBuffer* ib = nullptr;
        static VertexArray* va = nullptr;
        static Texture2d* diffuseTexture = nullptr;

        static bool isInited = false;
        static float time = 0.0f;
        static Transform transform{ };
        transform.set_scale({ 1.0f, 1.0f, 1.0f });

        static Geometry geom = load_geometry_from_obj(fileSystem->sync_load("assets\\geometry\\monke\\monke.obj", FileLoadMode::READ));

        if (!isInited)
        {
            renderer->add_render_command([&]()
            {
                vb = create_vertex_buffer<EngineConfig::DEFAULT_RENDERER_TYPE>(geom.vertices.data(), geom.vertices.size() * sizeof(GeometryVertex));
                vb->set_layout(BufferLayout::ElementContainer{
                    BufferElement{ ShaderDataType::Float3, false }, // Position
                    BufferElement{ ShaderDataType::Float3, false }, // Normal
                    BufferElement{ ShaderDataType::Float2, false }  // UV
                });
                ib = create_index_buffer<EngineConfig::DEFAULT_RENDERER_TYPE>(geom.ids.data(), geom.ids.size());
                va = create_vertex_array<EngineConfig::DEFAULT_RENDERER_TYPE>();
                va->set_vertex_buffer(vb);
                va->set_index_buffer(ib);

                diffuseTexture = create_texture_2d<EngineConfig::DEFAULT_RENDERER_TYPE>("assets\\materials\\metal_plate\\diffuse.png");
            });
            isInited = true;
        }
        else
        {
            // @NOTE : Currently using dummy key
            GeometryCommandKey key = 0;
            GeometryCommandData* data = renderer->add_geometry(key);
            data->trf = transform;
            data->va = va;
            data->diffuseTexture = diffuseTexture;
        }
    }

    void AlfinaEngineApplication::process_end_frame() noexcept
    {
        al_profile_function();

        fileSystem->remove_finished_jobs();
        {
            al_profile_scope("Print log buffer");
            debug::globalLogger->print_log_buffer();
        }
        {
            al_profile_scope("Print profile buffer");
            debug::globalLogger->print_profile_buffer();
        }
        frameCount++;
    }

    void AlfinaEngineApplication::app_quit() noexcept
    {
        window->quit();
    }
}
