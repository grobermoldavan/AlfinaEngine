
#include "engine/application.h"
#include "engine/utilities/utilities.h"

#define TEST_TIMINGS 0
#if TEST_TIMINGS
#   include <chrono>
#endif

namespace al
{
    template<typename Bindings>
    void application_run(Application<Bindings>* application, CommandLineArgs args)
    {
        application_default_create(application, args);
        if constexpr (HAS_CHECK(Bindings, create))
        {
            application->bindings.create(application, args);
        }
#if TEST_TIMINGS
        // Test code for timing ====================================
        using ClockT = std::chrono::steady_clock;
        using DtDuration = std::chrono::duration<float>;
        auto previousTime = ClockT::now();
        // =========================================================
#endif
        while(!application_should_quit(application))
        {
#if TEST_TIMINGS
            // Test code for timing ====================================
            auto currentTime = ClockT::now();
            auto dt = std::chrono::duration_cast<DtDuration>(currentTime - previousTime).count();
            previousTime = currentTime;
            printf("%f\n", 1.0f / dt);
            // =========================================================
#endif
            application_default_update(application);
            if constexpr (HAS_CHECK(Bindings, update))
            {
                application->bindings.update(application);
            }
            if (!platform_window_is_minimized(&application->window))
            {
                application_default_render(application);
                if constexpr (HAS_CHECK(Bindings, render))
                {
                    application->bindings.render(application);
                }
            }
        }
        application_default_destroy(application);
        if constexpr (HAS_CHECK(Bindings, destroy))
        {
            application->bindings.destroy(application);
        }
    }

    template<typename Bindings>
    ApplicationCreationData application_default_get_creation_data(Application<Bindings>* application)
    {
        return
        {
            .windowInitData =
            {
                .name = "Application Window",
                .isFullscreen = false,
                .isResizable = true,
                .width = 1024,
                .height = 768
            }
        };
    }

    template<typename Bindings>
    void application_default_create(Application<Bindings>* application, CommandLineArgs args)
    {
        ApplicationCreationData creationData;
        if constexpr (HAS_CHECK(Bindings, get_creation_data))
        {
            creationData = application->bindings.get_creation_data(application);
        }
        else
        {
            creationData = application_default_get_creation_data(application);
        }
        construct(&application->stack, EngineConfig::STACK_ALLOCATOR_MEMORY_SIZE, get_system_allocator_bindings());
        constexpr PoolAllocatorBucketDescription bucketDescriptions[EngineConfig::POOL_ALLOCATOR_MAX_BUCKETS] = 
        {
            memory_bucket_desc(8, EngineConfig::POOL_ALLOCATOR_MEMORY_SIZE),
        };
        construct(&application->pool, bucketDescriptions, get_system_allocator_bindings());
        platform_window_construct(&application->window, creationData.windowInitData);
        platform_window_set_resize_callback(&application->window, [application](){
            renderer_handle_resize(&application->renderer);
            if constexpr (HAS_CHECK(Bindings, handle_window_resize))
            {
                application->bindings.handle_window_resize(application);
            }
        });
        platform_input_construct(&application->input);
        {
            AllocatorBindings allocatorBindings = get_allocator_bindings(&application->pool);
            PlatformFile allShaders[] =
            {
                platform_file_load(allocatorBindings, platform_path("assets", "shaders", "geometry_pass.frag.spv"), PlatformFileLoadMode::READ),
                platform_file_load(allocatorBindings, platform_path("assets", "shaders", "geometry_pass.vert.spv"), PlatformFileLoadMode::READ),
            };
            defer(for (uSize it = 0; it < array_size(allShaders); it++) platform_file_unload(allocatorBindings, allShaders[it]));
            ImageAttachment renderProcessAttachments[] =
            {
                { .format = ImageAttachment::DEPTH_32F, .width = 0, .height = 0, },
                { .format = ImageAttachment::RGBA_8,    .width = 0, .height = 0, },
                { .format = ImageAttachment::RGB_32F,   .width = 0, .height = 0, },
                { .format = ImageAttachment::RGB_32F,   .width = 0, .height = 0, },
            };
            PlatformFile stageShaders[] = { allShaders[0], allShaders[1], };
            OutputAttachmentReference stageOutputs[] =
            {
                { .imageAttachmentIndex = 1, .name = "out_albedo", },
                { .imageAttachmentIndex = 2, .name = "out_position", },
                { .imageAttachmentIndex = 3, .name = "out_normal", },
            };
            DepthUsageInfo stageDepth { .imageAttachmentIndex = 0, };
            RenderStage renderProcessStages[] =
            {
                {
                    .shaders            = { .ptr = stageShaders, .size = array_size(stageShaders), },
                    .sampledAttachments = { .ptr = nullptr, .size = 0, },
                    .outputAttachments  = { .ptr = stageOutputs, .size = array_size(stageOutputs), },
                    .depth              = &stageDepth,
                    .stageDependencies  = { .ptr = nullptr, .size = 0, },
                },
            };
            RenderProcessDescription renderProcessDesccription
            {
                .imageAttachments           = { .ptr = renderProcessAttachments, .size = array_size(renderProcessAttachments), },
                .stages                     = { .ptr = renderProcessStages, .size = array_size(renderProcessStages), },
                .resultImageAttachmentIndex = 1,
            };
            RendererInitData rendererInitData
            {
                .bindings           = get_allocator_bindings(&application->pool),
                .window             = &application->window,
                .renderProcessDesc  = &renderProcessDesccription,
            };
            renderer_construct(&application->renderer, &rendererInitData);
        }
    }

    template<typename Bindings>
    void application_default_destroy(Application<Bindings>* application)
    {
        renderer_destruct(&application->renderer);
        platform_input_destruct(&application->input);
        platform_window_destruct(&application->window);
        destruct(&application->pool);
        destruct(&application->stack);
    }

    template<typename Bindings>
    void application_default_update(Application<Bindings>* application)
    {
        platform_window_process(&application->window);
        platform_input_update(&application->input);
    }

    template<typename Bindings>
    void application_default_render(Application<Bindings>* application)
    {
        renderer_render(&application->renderer);
    }

    template<typename Bindings>
    bool application_should_quit(Application<Bindings>* application)
    {
        if constexpr (HAS_CHECK(Bindings, should_quit))
        {
            return application->bindings.should_quit(application);
        }
        else
        {
            return platform_window_is_close_button_pressed(&application->window) || platform_input_is_keyboard_input_active(&application->input, KeyboardInput::ESCAPE);
        }
    }
}
