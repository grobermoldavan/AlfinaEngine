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
#include "engine/game_cameras/fly_camera.h"

#include "utilities/event.h"

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

        void dbg_render() noexcept;

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
}

#endif
