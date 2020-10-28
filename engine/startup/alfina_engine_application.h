#ifndef AL_ALFINA_ENGINE_APPLICATION_H
#define AL_ALFINA_ENGINE_APPLICATION_H

#include <span>

#include "engine/memory/memory_manager.h"
#include "engine/job_system/job_system.h"

namespace al::engine
{
    class AlfinaEngineApplication;

    using CommandLineParams = std::span<const char*>;

    extern AlfinaEngineApplication* create_application(CommandLineParams commandLine);  // @NOTE : this function is defined by user
    extern void destroy_application(AlfinaEngineApplication*);                          // @NOTE : this function is defined by user

    class AlfinaEngineApplication
    {
    public:
        virtual void InitializeComponents();
        virtual void TerminateComponents();
        virtual void Run();

    protected:
        MemoryManager memoryManager;
        JobSystem* jobSystem;
    };

    void AlfinaEngineApplication::InitializeComponents()
    {
        memoryManager.initialize();
        jobSystem = reinterpret_cast<JobSystem*>(memoryManager.get_stack()->allocate(sizeof(JobSystem)));
        ::new(jobSystem) JobSystem{ std::thread::hardware_concurrency(), memoryManager.get_stack() };
    }

    void AlfinaEngineApplication::TerminateComponents()
    {
        jobSystem->~JobSystem();
        memoryManager.terminate();
    }

    void AlfinaEngineApplication::Run()
    {
        
    }
}

#endif
