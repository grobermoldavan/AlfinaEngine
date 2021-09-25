
#include "engine/application.h"
#include "engine/utilities/utilities.h"

namespace al
{
    template<typename Bindings>
    void application_run(Application<Bindings>* application, CommandLineArgs args)
    {
        application_default_create(application, args);
        if constexpr (AL_HAS_CHECK(Bindings, application_create))
        {
            application->bindings.application_create((typename Bindings::ApplicationType*)application, args);
        }
        while(!application_should_quit(application))
        {
            application_default_update(application);
            if constexpr (AL_HAS_CHECK(Bindings, application_update))
            {
                application->bindings.application_update((typename Bindings::ApplicationType*)application);
            }
            if (!platform_window_is_minimized(&application->window))
            {
                renderer_default_render(&application->renderer);
                if constexpr (AL_HAS_CHECK(Bindings, renderer_render))
                {
                    application->bindings.renderer_render((typename Bindings::ApplicationType*)application);
                }
            }
            logger_flush(&application->logger);
            stack_alloactor_reset(&application->frameAllocator);
        }
        if constexpr (AL_HAS_CHECK(Bindings, application_destroy))
        {
            application->bindings.application_destroy((typename Bindings::ApplicationType*)application);
        }
        application_default_destroy(application);
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
                .isResizable = false,
                .width = 1024,
                .height = 768
            },
            .renderApi = RenderApi::VULKAN,
        };
    }

    template<typename Bindings>
    void application_default_create(Application<Bindings>* application, CommandLineArgs args)
    {
        ApplicationCreationData creationData;
        if constexpr (AL_HAS_CHECK(Bindings, get_creation_data))
        {
            creationData = application->bindings.get_creation_data((typename Bindings::ApplicationType*)application);
        }
        else
        {
            creationData = application_default_get_creation_data(application);
        }

        AllocatorBindings systemAllocatorBindings = get_system_allocator_bindings();
        construct(&application->stack, EngineConfig::STACK_ALLOCATOR_MEMORY_SIZE, &systemAllocatorBindings);
        constexpr PoolAllocatorBucketDescription bucketDescriptions[EngineConfig::POOL_ALLOCATOR_MAX_BUCKETS] = 
        {
            memory_bucket_desc(8, EngineConfig::POOL_ALLOCATOR_MEMORY_SIZE),
        };
        construct(&application->pool, bucketDescriptions, &systemAllocatorBindings);
        construct(&application->frameAllocator, EngineConfig::FRAME_ALLOCATOR_MEMORY_SIZE, &systemAllocatorBindings);
        application->stackBindings = get_allocator_bindings(&application->stack);
        application->poolBindings = get_allocator_bindings(&application->pool);
        application->frameBindings = get_allocator_bindings(&application->frameAllocator);

        PlatformFile stdOut;
        unwrap(platform_file_get_std_out(&stdOut));
        LoggerCreateInfo loggerCreateInfo
        {
            .persistentAllocator = application->poolBindings,
            .outputFiles = { &stdOut, 1 },
        };
        logger_construct(&application->logger, &loggerCreateInfo);

        application->globals =
        {
            .logger = &application->logger,
        };
        thread_local_globals_register(&application->globals);

        platform_window_construct(&application->window, creationData.windowInitData);
        platform_window_set_resize_callback(&application->window, [application](){
            renderer_default_handle_resize(&application->renderer);
            if constexpr (AL_HAS_CHECK(Bindings, handle_window_resize))
            {
                application->bindings.handle_window_resize((typename Bindings::ApplicationType*)application);
            }
        });
        platform_input_construct(&application->input);

        RendererInitData rendererInitData
        {
            .persistentAllocator    = get_allocator_bindings(&application->pool),
            .frameAllocator         = get_allocator_bindings(&application->frameAllocator),
            .window                 = &application->window,
            .renderApi              = creationData.renderApi,
        };
        renderer_default_construct(&application->renderer, &rendererInitData);
        if constexpr (AL_HAS_CHECK(Bindings, renderer_construct))
        {
            application->bindings.renderer_construct((typename Bindings::ApplicationType*)application);
        }
    }

    template<typename Bindings>
    void application_default_destroy(Application<Bindings>* application)
    {
        if constexpr (AL_HAS_CHECK(Bindings, renderer_destroy))
        {
            application->bindings.renderer_destroy((typename Bindings::ApplicationType*)application);
        }
        renderer_default_destroy(&application->renderer);
        platform_input_destruct(&application->input);
        platform_window_destruct(&application->window);
        logger_destroy(&application->logger);
        destruct(&application->pool);
        destruct(&application->stack);
        destruct(&application->frameAllocator);
    }

    template<typename Bindings>
    void application_default_update(Application<Bindings>* application)
    {
        platform_window_process(&application->window);
        platform_input_update(&application->input);
    }

    template<typename Bindings>
    bool application_should_quit(Application<Bindings>* application)
    {
        if constexpr (AL_HAS_CHECK(Bindings, should_quit))
        {
            return application->bindings.should_quit((typename Bindings::ApplicationType*)application);
        }
        else
        {
            return platform_window_is_close_button_pressed(&application->window) || platform_input_is_keyboard_input_active(&application->input, KeyboardInput::ESCAPE);
        }
    }
}
