#define AL_LOGGING_ENABLED
#define AL_PROFILING_ENABLED
#define AL_DEBUG

#include "engine/engine.h"

class UserApplication : public al::engine::AlfinaEngineApplication
{
public:
    virtual void initialize_components() noexcept override;
    virtual void terminate_components() noexcept override;

    virtual void render() noexcept override;

private:
    static constexpr const char* LOG_CATEGORY_USER_APPLICATION = "UserApp";

    al::engine::TextureResourceHandle tex{ 0 };
    al::engine::MeshResourceHandle mesh{ 0 };

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
    using namespace al::engine;
    AlfinaEngineApplication::initialize_components();
    al_log_message(LOG_CATEGORY_USER_APPLICATION, "Initializing user application components");

    onKeyboardButtonPressed.subscribe({ this, &UserApplication::handle_keyboard_input });
    onMouseButtonPressed.subscribe({ this, &UserApplication::handle_mouse_input });

    tex = ResourceManager::get()->add_texture_resource(construct_path("assets", "materials", "metal_plate", "diffuse.png"));
    mesh = ResourceManager::get()->add_mesh_resource(construct_path("assets", "geometry", "sponza", "sponza.obj"));
}

void UserApplication::terminate_components() noexcept
{
    al_log_message(LOG_CATEGORY_USER_APPLICATION, "Terminating user application components");

    onKeyboardButtonPressed.unsubscribe(this);
    onMouseButtonPressed.unsubscribe(this);

    al::engine::AlfinaEngineApplication::terminate_components();
}

void UserApplication::render() noexcept
{
    al_profile_function();
    using namespace al::engine;
    RenderMesh* renderMesh = ResourceManager::get()->get_render_mesh(mesh);
    renderMesh->submeshes.for_each([&](RenderSubmesh* submesh)
    {
        GeometryCommandData* data = Renderer::get()->add_geometry_command({ });
        data->trf = Transform{ };
        data->trf.set_position({ 0, 0, 0 });
        data->trf.set_scale({ 1, 1, 1 });
        data->va = Renderer::get()->vertex_array(submesh->vaHandle);
        data->diffuseTexture = Renderer::get()->texture_2d(ResourceManager::get()->get_renderer_texture_handle(tex));
    });
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
