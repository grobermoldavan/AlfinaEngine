
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
        if constexpr (AL_HAS_CHECK(Bindings, create))
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
            if constexpr (AL_HAS_CHECK(Bindings, update))
            {
                application->bindings.update(application);
            }
            if (!platform_window_is_minimized(&application->window))
            {
                application_default_render(application);
                if constexpr (AL_HAS_CHECK(Bindings, render))
                {
                    application->bindings.render(application);
                }
            }
            stack_alloactor_reset(&application->frameAllocator);
        }
        application_default_destroy(application);
        if constexpr (AL_HAS_CHECK(Bindings, destroy))
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
                .isResizable = false,
                .width = 1024,
                .height = 768
            }
        };
    }

    template<typename Bindings>
    void application_default_create(Application<Bindings>* application, CommandLineArgs args)
    {
        ApplicationCreationData creationData;
        if constexpr (AL_HAS_CHECK(Bindings, get_creation_data))
        {
            creationData = application->bindings.get_creation_data(application);
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
        platform_window_construct(&application->window, creationData.windowInitData);
        platform_window_set_resize_callback(&application->window, [application](){
            renderer_handle_resize(&application->renderer);
            if constexpr (AL_HAS_CHECK(Bindings, handle_window_resize))
            {
                application->bindings.handle_window_resize(application);
            }
        });
        platform_input_construct(&application->input);
        RendererInitData rendererInitData
        {
            .persistentAllocator    = get_allocator_bindings(&application->pool),
            .frameAllocator         = get_allocator_bindings(&application->frameAllocator),
            .window                 = &application->window,
            .renderApi              = RenderApi::VULKAN,
        };
        renderer_construct(&application->renderer, &rendererInitData);
    }

    template<typename Bindings>
    void application_default_destroy(Application<Bindings>* application)
    {
        renderer_destruct(&application->renderer);
        platform_input_destruct(&application->input);
        platform_window_destruct(&application->window);
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
    void application_default_render(Application<Bindings>* application)
    {
        renderer_render(&application->renderer);
    }

    template<typename Bindings>
    bool application_should_quit(Application<Bindings>* application)
    {
        if constexpr (AL_HAS_CHECK(Bindings, should_quit))
        {
            return application->bindings.should_quit(application);
        }
        else
        {
            return platform_window_is_close_button_pressed(&application->window) || platform_input_is_keyboard_input_active(&application->input, KeyboardInput::ESCAPE);
        }
    }
}
