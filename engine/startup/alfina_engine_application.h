#ifndef AL_ALFINA_ENGINE_APPLICATION_H
#define AL_ALFINA_ENGINE_APPLICATION_H

#include <span>

#include "engine/memory/memory_manager.h"
#include "engine/job_system/job_system.h"
#include "engine/window/os_window.h"
#include "engine/file_system/file_system.h"

#include "utilities/event.h"
#include "utilities/toggle.h"
#include "utilities/math.h"

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
        void render() noexcept;

    protected:
        MemoryManager memoryManager;
        JobSystem* jobSystem;
        FileSystem* fileSystem;
        OsWindow* window;

        Toggle<OsWindowInput> inputState;

        Event<void(OsWindowInput::KeyboardInputFlags)> onKeyboardButtonPressed;
        Event<void(OsWindowInput::KeyboardInputFlags)> onKeyboardButtonReleased;
        Event<void(OsWindowInput::MouseInputFlags)> onMouseButtonPressed;
        Event<void(OsWindowInput::MouseInputFlags)> onMouseButtonReleased;

        void app_quit() noexcept;
    };

    void AlfinaEngineApplication::initialize_components() noexcept
    {
        GlobalAssertsData::mainThreadId = std::this_thread::get_id();

        const std::size_t NUM_OF_JOB_THREADS = std::thread::hardware_concurrency() - 1;

        memoryManager.initialize();

        jobSystem = memoryManager.get_stack()->allocate_as<JobSystem>();
        ::new(jobSystem) JobSystem{ NUM_OF_JOB_THREADS, memoryManager.get_stack() };

        fileSystem = memoryManager.get_stack()->allocate_as<FileSystem>();
        ::new(fileSystem) FileSystem{ memoryManager.get_pool() };

        window = create_window({ }, memoryManager.get_stack());
    }

    void AlfinaEngineApplication::terminate_components() noexcept
    {
        destroy_window(window);
        fileSystem->~FileSystem();
        jobSystem->~JobSystem();
        memoryManager.terminate();
    }

    void AlfinaEngineApplication::run() noexcept
    {
        using ClockT = std::chrono::steady_clock;
        using DtDuration = std::chrono::duration<float>;

        auto previousTime = ClockT::now();
        while(true)
        {
            auto currentTime = ClockT::now();
            auto dt = std::chrono::duration_cast<DtDuration>(currentTime - previousTime).count();
            previousTime = currentTime;
            
            window->process();
            if (window->is_quit())
            {
                break;
            }

            update_input();
            simulate(dt);
            render();
        }
    }

    void AlfinaEngineApplication::update_input() noexcept
    {
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

    }

    void AlfinaEngineApplication::render() noexcept
    {

    }

    void AlfinaEngineApplication::app_quit() noexcept
    {
        window->quit();
    }
}

#endif
