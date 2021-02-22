
#include "alfina_engine_application.h"

#include "utilities/smooth_average.h"
#include "utilities/constexpr_functions.h"

namespace al::engine
{
    void AlfinaEngineApplication::initialize_components() noexcept
    {
        MemoryManager::construct();
        debug::Logger::construct();
        JobSystem::construct(get_number_of_job_system_threads());
        FileSystem::construct();
        {
            OsWindowParams params;
            params.isFullscreen = false;
            window = create_window(&params);
        }
        Renderer::allocate_space();
        Renderer::construct(EngineConfig::DEFAULT_RENDERER_TYPE, window);
        ResourceManager::construct();

        distribute_threads_to_cpu_cores();

        defaultEcsWorld = MemoryManager::get_stack()->allocate_as<EcsWorld>();
        ::new(defaultEcsWorld) EcsWorld{ };

        defaultScene = MemoryManager::get_stack()->allocate_as<Scene>();
        ::new(defaultScene) Scene{ defaultEcsWorld };

        dbgFlyCamera.get_render_camera()->set_aspect_ratio(static_cast<float>(window->get_params()->width) / static_cast<float>(window->get_params()->height));
        Renderer::get()->set_camera(dbgFlyCamera.get_render_camera());

        al_log_message(LOG_CATEGORY_BASE_APPLICATION, "Initialized engine components");
    }

    void AlfinaEngineApplication::terminate_components() noexcept
    {
        al_log_message(LOG_CATEGORY_BASE_APPLICATION, "Terminating engine components");

        defaultScene->~Scene();
        defaultEcsWorld->~EcsWorld();

        ResourceManager::destruct();
        Renderer::destruct();
        destroy_window(window);
        FileSystem::destruct();
        JobSystem::destruct();
        debug::Logger::destruct();
        MemoryManager::destruct();
    }

    void AlfinaEngineApplication::run() noexcept
    {
        using ClockT = std::chrono::steady_clock;
        using DtDuration = std::chrono::duration<float>;
        al_log_message(LOG_CATEGORY_BASE_APPLICATION, "Starting application");
        {
            al_profile_scope("Print log buffer");
            debug::Logger::print_log_buffer();
        }
        {
            al_profile_scope("Print profile buffer");
            debug::Logger::print_profile_buffer();
        }
        frameCount = 0;
        auto previousTime = ClockT::now();
        while(true)
        {
            al_profile_scope("Process frame");
            auto currentTime = ClockT::now();
            auto dt = std::chrono::duration_cast<DtDuration>(currentTime - previousTime).count();
            previousTime = currentTime;
            {
                al_profile_scope("Process window");
                window->process();
                if (window->is_quit())
                {
                    break;
                }
            }
            Renderer::get()->start_process_frame();
            update_input();
            simulate(dt);
            Renderer::get()->wait_for_command_buffers_toggled();
            dbg_render();
            process_end_frame();
            Renderer::get()->wait_for_render_finish();
        }
        defaultEcsWorld->log_world_state();
    }

    void AlfinaEngineApplication::update_input() noexcept
    {
        al_profile_function();

        inputState.toggle();
        OsWindowInput& current = inputState.get_current();
        OsWindowInput& previous = inputState.get_previous();

        current = window->get_input();

        for (std::size_t it = 1; it < static_cast<std::size_t>(OsWindowInput::KeyboardInputFlags::__end); it++)
        {
            bool currentFlag = current.keyboard.buttons.get_flag(it);
            bool previousFlag = previous.keyboard.buttons.get_flag(it);
            if (currentFlag && !previousFlag)
            {
                onKeyboardButtonPressed(static_cast<OsWindowInput::KeyboardInputFlags>(it));
            }
            else if (!currentFlag && previousFlag)
            {
                onKeyboardButtonReleased(static_cast<OsWindowInput::KeyboardInputFlags>(it));
            }
        }

        for (std::size_t it = 0; it < static_cast<std::size_t>(OsWindowInput::MouseInputFlags::__end); it++)
        {
            bool currentFlag = current.mouse.buttons.get_flag(it);
            bool previousFlag = previous.mouse.buttons.get_flag(it);
            if (currentFlag && !previousFlag)
            {
                onMouseButtonPressed(static_cast<OsWindowInput::MouseInputFlags>(it));
            }
            else if (!currentFlag && previousFlag)
            {
                onMouseButtonReleased(static_cast<OsWindowInput::MouseInputFlags>(it));
            }
        }
    }

    void AlfinaEngineApplication::simulate(float dt) noexcept
    {
        static SmoothAverage<float> fps;
        fps.push(dt);
        // al_log_message(LOG_CATEGORY_BASE_APPLICATION, "Fps : %f", 1.0f / fps.get());

        dbgFlyCamera.process_inputs(&inputState.get_current(), dt);
    }

    void AlfinaEngineApplication::dbg_render() noexcept
    {
        al_profile_function();

        // static VertexBuffer* vb = nullptr;
        // static IndexBuffer* ib = nullptr;
        // static VertexArray* va = nullptr;
        // static Texture2d* diffuseTexture = nullptr;
        // static bool isInited = false;

        static TextureResourceHandle tex{ 0 };
        static CpuMesh mesh;

        if (!tex.isValid)
        {
            tex = ResourceManager::get()->add_texture_resource(construct_path("assets", "materials", "metal_plate", "diffuse.png"));
            {
                auto [handle, loadJob] = FileSystem::get()->async_load(construct_path("assets", "geometry", "cube.obj"), FileLoadMode::READ);
                Job* postLoadJob = JobSystem::get()->get_job();
                CpuMesh* meshPtr = &mesh;
                postLoadJob->configure([handle, meshPtr](Job* job)
                {
                    al_log_message(LOG_CATEGORY_BASE_APPLICATION, "Loading mesh");
                    *meshPtr = load_cpu_mesh_obj(handle);
                    meshPtr->submeshes.for_each([](CpuSubmesh* submesh)
                    {
                        al_log_message(LOG_CATEGORY_BASE_APPLICATION, "Loaded submesh with name %s", submesh->name);
                    });
                    FileSystem::get()->free_handle(handle);
                });
                postLoadJob->set_after(loadJob);
                JobSystem::get()->start_job(postLoadJob);
            }
        }

        // if (!isInited)
        // {
        //     Renderer::get()->add_render_command([&]()
        //     {
        //         vb = create_vertex_buffer<EngineConfig::DEFAULT_RENDERER_TYPE>(geom.vertices.data(), geom.vertices.size() * sizeof(GeometryVertex));
        //         vb->set_layout(BufferLayout::ElementContainer{
        //             BufferElement{ ShaderDataType::Float3, false }, // Position
        //             BufferElement{ ShaderDataType::Float3, false }, // Normal
        //             BufferElement{ ShaderDataType::Float2, false }  // UV
        //         });
        //         ib = create_index_buffer<EngineConfig::DEFAULT_RENDERER_TYPE>(geom.ids.data(), geom.ids.size());
        //         va = create_vertex_array<EngineConfig::DEFAULT_RENDERER_TYPE>();
        //         va->set_vertex_buffer(vb);
        //         va->set_index_buffer(ib);
        //         diffuseTexture = create_texture_2d<EngineConfig::DEFAULT_RENDERER_TYPE>(construct_path("assets", "materials", "metal_plate", "diffuse.png"));
        //     });
        //     SceneNodeHandle parent;
        //     {
        //         parent = defaultScene->create_node();
        //         auto* parentTrf = defaultScene->get_ecs_world()->get_component<SceneTransform>(defaultScene->scene_node(parent)->entityHandle);
        //         Transform localTransform = parentTrf->get_local_transform();
        //         localTransform.set_scale({ 1.0f, 1.0f, 1.0f });
        //         parentTrf->set_local_transform(localTransform.matrix);
        //         EntityHandle entity = defaultScene->scene_node(parent)->entityHandle;
        //         defaultScene->get_ecs_world()->add_components<Renderable>(entity);
        //     }
        //     SceneNodeHandle child;
        //     {
        //         child = defaultScene->create_node();
        //         auto* childTrf = defaultScene->get_ecs_world()->get_component<SceneTransform>(defaultScene->scene_node(child)->entityHandle);
        //         Transform localTransform = childTrf->get_local_transform();
        //         localTransform.set_position({ -2, -2, -2 });
        //         childTrf->set_local_transform(localTransform.matrix);
        //         defaultScene->set_parent(child, parent);
        //         EntityHandle entity = defaultScene->scene_node(child)->entityHandle;
        //         defaultScene->get_ecs_world()->add_components<Renderable>(entity);
        //     }
        //     SceneNodeHandle child2;
        //     {
        //         child2 = defaultScene->create_node();
        //         auto* childTrf = defaultScene->get_ecs_world()->get_component<SceneTransform>(defaultScene->scene_node(child2)->entityHandle);
        //         Transform localTransform = childTrf->get_local_transform();
        //         localTransform.set_position({ -2, -2, -2 });
        //         childTrf->set_local_transform(localTransform.matrix);
        //         defaultScene->set_parent(child2, child);
        //         EntityHandle entity = defaultScene->scene_node(child2)->entityHandle;
        //         defaultScene->get_ecs_world()->add_components<Renderable>(entity);
        //     }
        //     isInited = true;
        // }
        // else if (vb && ib && va && diffuseTexture)
        // {
        //     defaultEcsWorld->for_each<SceneTransform>([&](EcsWorld* world, EntityHandle handle, SceneTransform* trf)
        //     {
        //         Transform localTrf = trf->get_local_transform();
        //         float3 rot = localTrf.get_rotation();
        //         rot.y += 1.f;
        //         localTrf.set_rotation(rot);
        //         trf->set_local_transform(localTrf.matrix);
        //     });
        //     defaultEcsWorld->for_each<SceneTransform, Renderable>([&](EcsWorld* world, EntityHandle handle, SceneTransform* trf, Renderable*)
        //     {
        //         GeometryCommandKey key = 0;
        //         GeometryCommandData* data = Renderer::get()->add_geometry_command(key);
        //         data->trf = trf->get_world_transform();
        //         data->va = va;
        //         data->diffuseTexture = diffuseTexture;
        //     });
        // }
    }

    void AlfinaEngineApplication::process_end_frame() noexcept
    {
        al_profile_function();
        defaultScene->update_transforms();
        {
            al_profile_scope("Print log buffer");
            debug::Logger::print_log_buffer();
        }
        {
            al_profile_scope("Print profile buffer");
            debug::Logger::print_profile_buffer();
        }
        frameCount++;
    }

    void AlfinaEngineApplication::app_quit() noexcept
    {
        window->quit();
    }

    std::size_t AlfinaEngineApplication::get_number_of_job_system_threads() noexcept
    {
        const std::size_t systemMaxThreads = std::thread::hardware_concurrency();
        if (systemMaxThreads > 2)
        {
            // Minus one for main thread and minus one for render thread
            return systemMaxThreads - 2;
        }
        else
        {
            return 1;   
        }
    }

    void AlfinaEngineApplication::distribute_threads_to_cpu_cores() noexcept
    {
        // Collect handles to all threads used by the program
        ArrayContainer<ThreadHandle, EngineConfig::MAX_SUPPORTED_THREADS> threads;
        threads.push(get_current_thread_handle());
        threads.push(Renderer::get()->get_render_thread()->native_handle());
        std::span<JobSystemThread> jobSystemThread = JobSystem::get()->get_threads();
        for (JobSystemThread& jobSystemThread : jobSystemThread)
        {
            threads.push(jobSystemThread.get_thread()->native_handle());
        }
        // Get max number of threads supported by the user's computer
        const std::size_t systemMaxThreads = std::thread::hardware_concurrency();
        // Get number of program threads
        const std::size_t programThreadsCount = threads.get_current_size();
        // Print warnings if needed
        if (systemMaxThreads > EngineConfig::MAX_SUPPORTED_THREADS)
        {
            al_log_warning(LOG_CATEGORY_BASE_APPLICATION, "Current system supports %d threads", systemMaxThreads);
            al_log_warning(LOG_CATEGORY_BASE_APPLICATION, "This is more than engine currntly supports (%d threads)", EngineConfig::MAX_SUPPORTED_THREADS);
        }
        if (programThreadsCount > EngineConfig::MAX_SUPPORTED_THREADS)
        {
            al_log_warning(LOG_CATEGORY_BASE_APPLICATION, "Using more threads than engine supports. Some threads will share the same cores");
        }
        if (programThreadsCount > systemMaxThreads)
        {
            al_log_warning(LOG_CATEGORY_BASE_APPLICATION, "Using more threads than system supports. Some threads will share the same cores");
        }
        // Get max supported number of threads by the program
        // If user's computer supports less than EngineConfig::MAX_SUPPORTED_THREADS threads, systemMaxThreads will be used
        // If user's computer supports more than EngineConfig::MAX_SUPPORTED_THREADS threads, EngineConfig::MAX_SUPPORTED_THREADS will be used
        const uint64_t maxThreads = minimum(systemMaxThreads, EngineConfig::MAX_SUPPORTED_THREADS);
        for (uint64_t it = 0; it < programThreadsCount; it++)
        {
            const uint64_t mask = uint64_t{1} << (it % maxThreads);
            set_thread_affinity_mask(threads[it], mask);
        }
    }
}
