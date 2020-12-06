#define AL_PLATFORM_WIN32

#define AL_LOGGING_ENABLED
#define AL_PROFILING_ENABLED

#include "engine/engine.h"

class UserApplication : public al::engine::AlfinaEngineApplication
{
public:
    virtual void initialize_components() noexcept override;
    virtual void terminate_components() noexcept override;

private:
    static constexpr const char* LOG_CATEGORY_USER_APPLICATION = "UserApp";

    void handle_keyboard_input(al::engine::OsWindowInput::KeyboardInputFlags);
    void handle_mouse_input(al::engine::OsWindowInput::MouseInputFlags);
};

UserApplication applicationInstance;

al::engine::AlfinaEngineApplication* al::engine::create_application(al::engine::CommandLineParams commandLine) noexcept
{
    return &applicationInstance;
}

void al::engine::destroy_application(al::engine::AlfinaEngineApplication* application) noexcept
{
    
}

void UserApplication::initialize_components() noexcept
{
    al::engine::AlfinaEngineApplication::initialize_components();

    al_log_message(LOG_CATEGORY_USER_APPLICATION, "Initializing user application components");

    onKeyboardButtonPressed.subscribe({ this, &UserApplication::handle_keyboard_input });
    onMouseButtonPressed.subscribe({ this, &UserApplication::handle_mouse_input });
}

void UserApplication::terminate_components() noexcept
{
    al_log_message(LOG_CATEGORY_USER_APPLICATION, "Terminating user application components");

    al::engine::AlfinaEngineApplication::terminate_components();
}

void UserApplication::handle_keyboard_input(al::engine::OsWindowInput::KeyboardInputFlags key)
{
    al_log_message(LOG_CATEGORY_USER_APPLICATION, "Pressed button %s", al::engine::OsWindowInput::KEYBOARD_FLAGS_TO_STRING[static_cast<uint32_t>(key)]);

    if (key == al::engine::OsWindowInput::KeyboardInputFlags::ESCAPE)
    {
        app_quit();
    }
}

void UserApplication::handle_mouse_input(al::engine::OsWindowInput::MouseInputFlags key)
{
    al_log_message(LOG_CATEGORY_USER_APPLICATION, "Pressed button %s", al::engine::OsWindowInput::MOUSE_FLAGS_TO_STRING[static_cast<uint32_t>(key)]);
}
