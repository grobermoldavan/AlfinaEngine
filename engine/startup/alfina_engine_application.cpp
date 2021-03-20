
#include "alfina_engine_application.h"

#include "utilities/smooth_average.h"
#include "utilities/constexpr_functions.h"

namespace al::engine
{
    void AlfinaEngineApplication::initialize_components() noexcept
    {
        gMemoryManager = &memoryManager;
        construct(gMemoryManager);

        gLogger = static_cast<Logger*>(allocate(&gMemoryManager->stack, sizeof(Logger)));
        construct(gLogger);

        gMainJobSystem = static_cast<JobSystem*>(allocate(&gMemoryManager->stack, sizeof(JobSystem)));
        construct(gMainJobSystem, get_number_of_job_system_threads());

        gRenderJobSystem = static_cast<JobSystem*>(allocate(&gMemoryManager->stack, sizeof(JobSystem)));
        construct(gRenderJobSystem, 0);

        construct_jobs();

        gFileSystem = static_cast<FileSystem*>(allocate(&gMemoryManager->stack, sizeof(FileSystem)));
        construct(gFileSystem);
        {
            OsWindowParams params;
            params.isFullscreen = false;
            window = create_window(&params);
        }
        Renderer::allocate_space();
        Renderer::construct_renderer(EngineConfig::DEFAULT_RENDERER_TYPE, window);
        ResourceManager::construct_manager();

        distribute_threads_to_cpu_cores();

        defaultEcsWorld = static_cast<EcsWorld*>(allocate(&gMemoryManager->stack, sizeof(EcsWorld)));
        construct(defaultEcsWorld);

        defaultScene = static_cast<Scene*>(allocate(&gMemoryManager->stack, sizeof(Scene)));
        wrap_construct(defaultScene, defaultEcsWorld);

        construct(&dbgFlyCamera);
        get_render_camera(&dbgFlyCamera)->set_aspect_ratio(static_cast<float>(window->get_params()->width) / static_cast<float>(window->get_params()->height));
        Renderer::get()->set_camera(get_render_camera(&dbgFlyCamera));

        // MemoryManager::log_memory_init_info();

        al_log_message(LOG_CATEGORY_BASE_APPLICATION, "Initialized engine components");
    }

    void AlfinaEngineApplication::terminate_components() noexcept
    {
        al_log_message(LOG_CATEGORY_BASE_APPLICATION, "Terminating engine components");

        wrap_destruct(defaultScene);
        destruct(defaultEcsWorld);

        ResourceManager::destruct();
        Renderer::destruct();
        destroy_window(window);
        destruct(gFileSystem);
        destruct(gMainJobSystem);
        destruct(gRenderJobSystem);
        destruct(gLogger);
        destruct(gMemoryManager);
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
            Renderer::get()->start_process_frame();
            update_input();
            simulate(dt);
            Renderer::get()->wait_for_command_buffers_toggled();
            render();
            process_end_frame();
            Renderer::get()->wait_for_render_finish();
        }
        // ecs_log_world_state(defaultEcsWorld);
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
        al_profile_function();
        al_log_message(LOG_CATEGORY_BASE_APPLICATION, "Fps : %f", 1.0f / dt);
        process_inputs(&dbgFlyCamera, &inputState.get_current(), dt);
    }

    void AlfinaEngineApplication::render() noexcept
    {
        al_profile_function();
    }

    void AlfinaEngineApplication::process_end_frame() noexcept
    {
        al_profile_function();
        defaultScene->update_transforms();
        logger_flush_buffers(gLogger);
        frameCount++;
    }

    void AlfinaEngineApplication::app_quit() noexcept
    {
        window->quit();
    }

    std::size_t AlfinaEngineApplication::get_number_of_job_system_threads() noexcept
    {
        const std::size_t systemMaxThreads = std::thread::hardware_concurrency();
        if (systemMaxThreads > 2)
        {
            // Minus one for main thread and minus one for render thread
            return systemMaxThreads - 2;
        }
        else
        {
            return 1;   
        }
    }

    void AlfinaEngineApplication::distribute_threads_to_cpu_cores() noexcept
    {
        // Collect handles to all threads used by the program
        ArrayContainer<ThreadHandle, EngineConfig::MAX_SUPPORTED_THREADS> threads{ };
        al_memzero(&threads);
        push(&threads, get_current_thread_handle());
        push(&threads, Renderer::get()->get_render_thread()->native_handle());
        std::span<JobSystemThread> jobSystemThread = gMainJobSystem->threads;
        for (JobSystemThread& jobSystemThread : jobSystemThread)
        {
            push(&threads, jobSystemThread.thread.native_handle());
        }
        // Get max number of threads supported by the user's computer
        const std::size_t systemMaxThreads = std::thread::hardware_concurrency();
        // Get number of program threads
        const std::size_t programThreadsCount = threads.size;
        // Print warnings if needed
        if (systemMaxThreads > EngineConfig::MAX_SUPPORTED_THREADS)
        {
            al_log_warning(LOG_CATEGORY_BASE_APPLICATION, "Current system supports %d threads", systemMaxThreads);
            al_log_warning(LOG_CATEGORY_BASE_APPLICATION, "This is more than engine currntly supports (%d threads)", EngineConfig::MAX_SUPPORTED_THREADS);
        }
        if (programThreadsCount > EngineConfig::MAX_SUPPORTED_THREADS)
        {
            al_log_warning(LOG_CATEGORY_BASE_APPLICATION, "Using more threads than engine supports. Some threads will share the same cores");
        }
        if (programThreadsCount > systemMaxThreads)
        {
            al_log_warning(LOG_CATEGORY_BASE_APPLICATION, "Using more threads than system supports. Some threads will share the same cores");
        }
        // Get max supported number of threads by the program
        // If user's computer supports less than EngineConfig::MAX_SUPPORTED_THREADS threads, systemMaxThreads will be used
        // If user's computer supports more than EngineConfig::MAX_SUPPORTED_THREADS threads, EngineConfig::MAX_SUPPORTED_THREADS will be used
        const uint64_t maxThreads = minimum(systemMaxThreads, EngineConfig::MAX_SUPPORTED_THREADS);
        for (uint64_t it = 0; it < programThreadsCount; it++)
        {
            const uint64_t mask = uint64_t{1} << (it % maxThreads);
            set_thread_affinity_mask(*get(&threads, it), mask);
            set_thread_highest_priority(*get(&threads, it));
        }
    }
}
