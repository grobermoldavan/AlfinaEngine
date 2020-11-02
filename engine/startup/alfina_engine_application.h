#ifndef AL_ALFINA_ENGINE_APPLICATION_H
#define AL_ALFINA_ENGINE_APPLICATION_H

#include <span>

#include "engine/memory/memory_manager.h"
#include "engine/job_system/job_system.h"
#include "engine/window/os_window.h"

namespace al::engine
{
    class AlfinaEngineApplication;

    using CommandLineParams = std::span<const char*>;

    extern AlfinaEngineApplication* create_application(CommandLineParams commandLine) noexcept; // @NOTE : this function is defined by user
    extern void destroy_application(AlfinaEngineApplication*) noexcept;                         // @NOTE : this function is defined by user

    class AlfinaEngineApplication
    {
    public:
        virtual void InitializeComponents() noexcept;
        virtual void TerminateComponents() noexcept;

        void Run() noexcept;
        void UpdateInput() noexcept;
        void Simulate(float dt) noexcept;
        void Render() noexcept;

    protected:
        MemoryManager memoryManager;
        JobSystem* jobSystem;
        OsWindow* window;
    };

    void AlfinaEngineApplication::InitializeComponents() noexcept
    {
        memoryManager.initialize();
        jobSystem = memoryManager.get_stack()->allocate_as<JobSystem>();
        ::new(jobSystem) JobSystem{ std::thread::hardware_concurrency() - 1, memoryManager.get_stack() };
        window = create_window({ }, memoryManager.get_stack());
    }

    void AlfinaEngineApplication::TerminateComponents() noexcept
    {
        destroy_window(window);
        jobSystem->~JobSystem();
        memoryManager.terminate();
    }

    void AlfinaEngineApplication::Run() noexcept
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

            UpdateInput();
            Simulate(dt);
            Render();
        }
    }

    void AlfinaEngineApplication::UpdateInput() noexcept
    {
        auto input = window->get_input();

    }

    void AlfinaEngineApplication::Simulate(float dt) noexcept
    {

    }

    void AlfinaEngineApplication::Render() noexcept
    {

    }
}

#endif
