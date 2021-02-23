#define AL_LOGGING_ENABLED
#define AL_PROFILING_ENABLED
#define AL_DEBUG

#include "engine/engine.h"

class UserApplication : public al::engine::AlfinaEngineApplication
{
public:
    virtual void initialize_components() noexcept override;
    virtual void terminate_components() noexcept override;

    virtual void simulate(float dt) noexcept override;

private:
    static constexpr const char* LOG_CATEGORY_USER_APPLICATION = "UserApp";

    al::engine::TextureResourceHandle tex{ 0 };
    al::engine::CpuMesh mesh{ };

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

    onKeyboardButtonPressed.unsubscribe(this);
    onMouseButtonPressed.unsubscribe(this);

    // @NOTE : currently this is needed because of DynamicArray memory deallocation being too late
    //         if it is done "automatically" on applicationInstance destruction
    mesh.~CpuMesh();

    al::engine::AlfinaEngineApplication::terminate_components();
}

void UserApplication::simulate(float dt) noexcept
{
    al_profile_function();
    using namespace al::engine;
    AlfinaEngineApplication::simulate(dt);
    if (!tex.isValid)
    {
        tex = ResourceManager::get()->add_texture_resource(construct_path("assets", "materials", "metal_plate", "diffuse.png"));
        {
            auto [handle, loadJob] = FileSystem::get()->async_load(construct_path("assets", "geometry", "sponza", "sponza.obj"), FileLoadMode::READ);
            Job* postLoadJob = JobSystem::get()->get_job();
            CpuMesh* meshPtr = &mesh;
            postLoadJob->configure([handle, meshPtr](Job* job)
            {
                al_log_message(LOG_CATEGORY_USER_APPLICATION, "Loading mesh");
                *meshPtr = load_cpu_mesh_obj(handle);
                meshPtr->submeshes.for_each([](CpuSubmesh* submesh)
                {
                    al_log_message(LOG_CATEGORY_USER_APPLICATION, "Loaded submesh with name %s", submesh->name);
                });
                FileSystem::get()->free_handle(handle);
            });
            postLoadJob->set_after(loadJob);
            JobSystem::get()->start_job(postLoadJob);
        }
    }
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
