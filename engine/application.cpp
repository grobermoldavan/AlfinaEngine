
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
            PlatformFile vertexShader = platform_file_load(allocatorBindings, platform_path("assets", "shaders", "vert.spv"), PlatformFileLoadMode::READ);
            PlatformFile fragmentShader = platform_file_load(allocatorBindings, platform_path("assets", "shaders", "frag.spv"), PlatformFileLoadMode::READ);
            defer(platform_file_unload(allocatorBindings, vertexShader));
            defer(platform_file_unload(allocatorBindings, fragmentShader));
            AttachmentDescription attachments[] =
            {
                SCREEN_FRAMEBUFFER_DESC,
            };
            AttachmentDependency outAttachments[] =
            {
                {
                    .attachmentIndex = 0,
                    .flags = 0
                }
            };
            RenderStageDescription renderStages[] =
            {
                {   // This pass takes no input framebuffer and outputs to swap chain framebuffer and it renders all passed geometry
                    .vertexShader           = vertexShader,
                    .fragmentShader         = fragmentShader,
                    .inAttachmentsSize      = 0,
                    .inAttachments          = nullptr,
                    .outAttachmentsSize     = array_size(outAttachments),
                    .outAttachments         = outAttachments,
                    .flags                  = RenderStageDescription::FLAG_PASS_GEOMETRY
                },
            };
            RenderProcessDescription process
            {
                .attachmentsSize    = array_size(attachments),
                .attachments        = attachments,
                .renderStagesSize   = array_size(renderStages),
                .renderStages       = renderStages,
            };
            RendererInitData rendererInitData
            {
                .bindings           = get_allocator_bindings(&application->pool),
                .window             = &application->window,
                .renderProcessDesc  = &process,
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
