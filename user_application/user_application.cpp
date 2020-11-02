#define AL_PLATFORM_WIN32
#define AL_DEBUG

#include "engine/engine.h"

class UserApplication : public al::engine::AlfinaEngineApplication
{
    virtual void InitializeComponents() noexcept override;
    virtual void TerminateComponents() noexcept override;
};

UserApplication applicationInstance;

al::engine::AlfinaEngineApplication* al::engine::create_application(al::engine::CommandLineParams commandLine) noexcept
{
    printf("Creating.\n");
    return &applicationInstance;
}

void al::engine::destroy_application(al::engine::AlfinaEngineApplication* application) noexcept
{
    printf("Destroying.\n");
}

void UserApplication::InitializeComponents() noexcept
{
    printf("Initializing.\n");
    al::engine::AlfinaEngineApplication::InitializeComponents();
}

void UserApplication::TerminateComponents() noexcept
{
    printf("Terminating.\n");
    al::engine::AlfinaEngineApplication::TerminateComponents();
}
