#define AL_PLATFORM_WIN32
#define AL_DEBUG

#include "engine/engine.h"

class UserApplication : public al::engine::AlfinaEngineApplication
{
    virtual void InitializeComponents() override;
    virtual void TerminateComponents() override;
    virtual void Run() override;
};

UserApplication applicationInstance;

al::engine::AlfinaEngineApplication* al::engine::create_application(al::engine::CommandLineParams commandLine)
{
    printf("Creating.\n");
    return &applicationInstance;
}

void al::engine::destroy_application(al::engine::AlfinaEngineApplication* application)
{
    printf("Destroying.\n");
}

void UserApplication::InitializeComponents()
{
    printf("Initializing.\n");
    al::engine::AlfinaEngineApplication::InitializeComponents();
}

void UserApplication::TerminateComponents()
{
    printf("Terminating.\n");
    al::engine::AlfinaEngineApplication::TerminateComponents();
}

void job_func(al::engine::Job*)
{
    printf("job from %d\n", std::this_thread::get_id());
    std::this_thread::sleep_for(std::chrono::milliseconds{ 100 });
}

void UserApplication::Run()
{
    printf("Running.\n");
    al::engine::AlfinaEngineApplication::Run();

#if 1
    al::engine::Job jobs[100];
    auto* root = &jobs[0];
    ::new(root) al::engine::Job{ job_func };
    jobSystem->add_job(root);
    for (int i = 1; i < 100; i++)
    {
        auto* job = &jobs[i];
        ::new(job) al::engine::Job{ job_func, root };
        jobSystem->add_job(job);
    }
    jobSystem->wait_for(root);
    printf("Finished running.\n");
#endif
}
