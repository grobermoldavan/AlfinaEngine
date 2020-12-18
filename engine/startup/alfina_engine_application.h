#ifndef AL_ALFINA_ENGINE_APPLICATION_H
#define AL_ALFINA_ENGINE_APPLICATION_H

#include <span>
#include <vector>

#include "engine/memory/memory_manager.h"
#include "engine/job_system/job_system.h"
#include "engine/window/os_window.h"
#include "engine/file_system/file_system.h"
#include "engine/debug/debug.h"
#include "engine/rendering/renderer.h"
#include "engine/rendering/camera/perspective_render_camera.h"
#include "engine/game_cameras/fly_camera.h"

#include "utilities/event.h"
#include "utilities/smooth_average.h"

namespace al::engine
{
    class AlfinaEngineApplication;

    using CommandLineParams = std::span<const char*>;

    extern AlfinaEngineApplication* create_application(CommandLineParams commandLine) noexcept; // @NOTE : this function is defined by user
    extern void destroy_application(AlfinaEngineApplication*) noexcept;                         // @NOTE : this function is defined by user

    class AlfinaEngineApplication
    {
    public:
        virtual void initialize_components() noexcept;
        virtual void terminate_components() noexcept;

        void run() noexcept;
        void update_input() noexcept;
        void simulate(float dt) noexcept;
        void process_end_frame() noexcept;

        void dbg_render_cube() noexcept;

    protected:
        static constexpr const char* LOG_CATEGORY_BASE_APPLICATION = "Engine";

        JobSystem* jobSystem;
        FileSystem* fileSystem;
        OsWindow* window;
        Renderer* renderer;

        Toggle<OsWindowInput> inputState;
        std::uint64_t frameCount;

        FlyCamera dbgFlyCamera;

        Event<void(OsWindowInput::KeyboardInputFlags)> onKeyboardButtonPressed;
        Event<void(OsWindowInput::KeyboardInputFlags)> onKeyboardButtonReleased;
        Event<void(OsWindowInput::MouseInputFlags)> onMouseButtonPressed;
        Event<void(OsWindowInput::MouseInputFlags)> onMouseButtonReleased;

        void app_quit() noexcept;
    };

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

        window = create_window({ });

        renderer = create_renderer<EngineConfig::DEFAULT_RENDERER_TYPE>(window);

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
            dbg_render_cube();
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
        al_log_message(LOG_CATEGORY_BASE_APPLICATION, "Fps : %f", 1.0f / fps.get());

        dbgFlyCamera.process_inputs(&inputState.get_current(), dt);
    }

    void AlfinaEngineApplication::dbg_render_cube() noexcept
    {
        al_profile_function();

        static VertexBuffer* vb = nullptr;
        static IndexBuffer* ib = nullptr;
        static VertexArray* va = nullptr;
        static Shader* shader = nullptr;

        static uint32_t indices[] = {
            0, 1, 3,
            3, 1, 2,
            1, 5, 2,
            2, 5, 6,
            5, 4, 6,
            6, 4, 7,
            4, 0, 7,
            7, 0, 3,
            3, 2, 7,
            7, 2, 6,
            4, 5, 0,
            0, 5, 1
        };

        static float vertices[] =  {
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f
        };

        static bool isInited = false;
        static float time = 0.0f;
        static Transform cubeTransform{ IDENTITY4 };

        static FileHandle* vertSrc = fileSystem->async_load("assets\\shaders\\vertex.vert", FileLoadMode::READ);
        static FileHandle* fragSrc = fileSystem->async_load("assets\\shaders\\fragment.frag", FileLoadMode::READ);
        static bool isShadersLoadedChache = false;

        static auto isShadersLoaded = [&]() -> bool
        {
            if (isShadersLoadedChache)
            {
                return true;
            }
            else
            {
                isShadersLoadedChache = (vertSrc->state == FileHandle::State::LOADED) && (fragSrc->state == FileHandle::State::LOADED);
                return isShadersLoadedChache;
            }
        };

        if (!isInited && isShadersLoaded())
        {
            renderer->add_render_command([&]()
            {
                vb = create_vertex_buffer<RendererType::OPEN_GL>(vertices, sizeof(vertices));
                vb->set_layout(BufferLayout::ElementContainer{ BufferElement{ ShaderDataType::Float3, false } });
                ib = create_index_buffer<RendererType::OPEN_GL>(indices, sizeof(indices) / sizeof(uint32_t));
                va = create_vertex_array<RendererType::OPEN_GL>();
                va->set_vertex_buffer(vb);
                va->set_index_buffer(ib);

                const char* v = reinterpret_cast<const char*>(vertSrc->memory);
                const char* f = reinterpret_cast<const char*>(fragSrc->memory);
                shader = create_shader<RendererType::OPEN_GL>(v, f);

                fileSystem->free_handle(vertSrc);
                fileSystem->free_handle(fragSrc);
            });

            isInited = true;
        }
        else if (isShadersLoaded())
        {
            // @NOTE : Currently using dummy key
            DrawCommandKey key = 0;
            DrawCommandData* data = renderer->add_draw_command(key);
            data->trf = cubeTransform;
            data->va = va;
            data->shader = shader;
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

#endif
