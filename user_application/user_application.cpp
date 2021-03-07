
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

    // Add scene node with render mesh component
    SceneNodeHandle testSceneNode;
    testSceneNode = defaultScene->create_node();
    EntityHandle entity = defaultScene->scene_node(testSceneNode)->entityHandle;
    EcsWorld* world = defaultScene->get_ecs_world();
    world->add_components<RenderMeshComponent>(entity);
    RenderMeshComponent* meshComponent = world->get_component<RenderMeshComponent>(entity);
    meshComponent->resourceHandle = mesh;
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
    defaultEcsWorld->for_each<SceneTransform, RenderMeshComponent>([&](EcsWorld* world, EntityHandle handle, SceneTransform* trf, RenderMeshComponent* mesh)
    {
        ResourceManager::get()->get_render_mesh(mesh->resourceHandle)->submeshes.for_each([&](RenderSubmesh* submesh)
        {
            if (submesh->vaHandle.isValid)
            {
                GeometryCommandData* data = Renderer::get()->add_geometry_command({ /* dummy key */ });
                data->trf = trf->get_world_transform();
                data->va = Renderer::get()->vertex_array(submesh->vaHandle);
                data->diffuseTexture = Renderer::get()->texture_2d(ResourceManager::get()->get_renderer_texture_handle(tex));
            }
        });
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
