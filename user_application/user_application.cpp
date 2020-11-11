#define AL_PLATFORM_WIN32
#define AL_DEBUG

#include "engine/engine.h"

class UserApplication : public al::engine::AlfinaEngineApplication
{
public:
    virtual void initialize_components() noexcept override;
    virtual void terminate_components() noexcept override;

private:
    void handle_keyboard_input(al::engine::OsWindowInput::KeyboardInputFlags);
    void handle_mouse_input(al::engine::OsWindowInput::MouseInputFlags);
};

UserApplication applicationInstance;

al::engine::AlfinaEngineApplication* al::engine::create_application(al::engine::CommandLineParams commandLine) noexcept
{
    std::cout << "Creating.\n";
    return &applicationInstance;
}

void al::engine::destroy_application(al::engine::AlfinaEngineApplication* application) noexcept
{
    std::cout << "Destroying.\n";
}

void UserApplication::initialize_components() noexcept
{
    std::cout << "Initializing.\n";
    al::engine::AlfinaEngineApplication::initialize_components();

    onKeyboardButtonPressed.subscribe({ this, &UserApplication::handle_keyboard_input });
    onMouseButtonPressed.subscribe({ this, &UserApplication::handle_mouse_input });
}

void UserApplication::terminate_components() noexcept
{
    std::cout << "Terminating.\n";
    al::engine::AlfinaEngineApplication::terminate_components();
}

void UserApplication::handle_keyboard_input(al::engine::OsWindowInput::KeyboardInputFlags key)
{
    std::cout << al::engine::OsWindowInput::KEYBOARD_FLAGS_TO_STRING[static_cast<uint32_t>(key)] << std::endl;

    if (key == al::engine::OsWindowInput::KeyboardInputFlags::ESCAPE)
    {
        app_quit();
    }
}

void UserApplication::handle_mouse_input(al::engine::OsWindowInput::MouseInputFlags key)
{
    std::cout << al::engine::OsWindowInput::MOUSE_FLAGS_TO_STRING[static_cast<uint32_t>(key)] << std::endl;
}
