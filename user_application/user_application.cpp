
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

    tex = ResourceManager::get_instance()->add_texture_resource(construct_path("assets", "materials", "metal_plate", "diffuse.png"));
    mesh = ResourceManager::get_instance()->add_mesh_resource(construct_path("assets", "geometry", "sponza", "sponza.obj"));

    // Add scene node with render mesh component
    SceneNodeHandle testSceneNode;
    testSceneNode = defaultScene->create_node();
    EcsEntityHandle entity = defaultScene->scene_node(testSceneNode)->entityHandle;
    EcsWorld* world = defaultScene->get_ecs_world();
    ecs_add_components<RenderMeshComponent>(world, entity);
    RenderMeshComponent* meshComponent = ecs_get_component<RenderMeshComponent>(world, entity);
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
    ecs_for_each<SceneTransform, RenderMeshComponent>(defaultEcsWorld, [&](EcsWorld* world, EcsEntityHandle handle, SceneTransform* trf, RenderMeshComponent* mesh)
    {
        RenderMesh* renderMesh = ResourceManager::get_instance()->get_render_mesh(mesh->resourceHandle);
        for_each_array_container(renderMesh->submeshes, it)
        {
            RenderSubmesh* submesh = get(&renderMesh->submeshes, it);
            if (submesh->vaHandle.isValid)
            {
                GeometryCommandData* data = Renderer::get()->add_geometry_command({ /* dummy key */ });
                data->trf = trf->get_world_transform();
                // @TODO :  It is better to directly store pointer to a VA instead of handle because this will be faster
                data->va = Renderer::get()->vertex_array(submesh->vaHandle);
                // @TODO :  Same here. Better store diffuse texture pointer instead of getting it from handle which we got from another handle
                data->diffuseTexture = Renderer::get()->texture_2d(ResourceManager::get_instance()->get_renderer_texture_handle(tex));
            }
        };
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
