#ifndef AL_APPLICATION_H
#define AL_APPLICATION_H

#include "engine/memory/memory.h"
#include "engine/platform/platform.h"
#include "engine/render/renderer.h"
#include "engine/utilities/utilities.h"

namespace al
{
    AL_HAS_CHECK_DECLARATION(application_create);
    AL_HAS_CHECK_DECLARATION(application_destroy);
    AL_HAS_CHECK_DECLARATION(application_update);
    AL_HAS_CHECK_DECLARATION(renderer_construct);
    AL_HAS_CHECK_DECLARATION(renderer_destroy);
    AL_HAS_CHECK_DECLARATION(renderer_render);
    AL_HAS_CHECK_DECLARATION(should_quit);
    AL_HAS_CHECK_DECLARATION(handle_window_resize);
    AL_HAS_CHECK_DECLARATION(get_creation_data);

    struct CommandLineArgs
    {
        int argc;
        char** argv;
    };

    template<typename Bindings>
    struct Application
    {
        StackAllocator  stack;
        PoolAllocator   pool;
        StackAllocator  frameAllocator;

        AllocatorBindings stackBindings;
        AllocatorBindings poolBindings;
        AllocatorBindings frameBindings;

        PlatformWindow  window;
        PlatformInput   input;
        Renderer        renderer;
        Logger          logger;
        Bindings        bindings;
    };

    struct ApplicationCreationData
    {
        PlatformWindowInitData windowInitData;
        RenderApi renderApi;
    };

    template<typename Bindings> void application_run(Application<Bindings>* application, CommandLineArgs args);

    template<typename Bindings> ApplicationCreationData application_default_get_creation_data   (Application<Bindings>* application);
    template<typename Bindings> void                    application_default_create              (Application<Bindings>* application, CommandLineArgs args);
    template<typename Bindings> void                    application_default_destroy             (Application<Bindings>* application);
    template<typename Bindings> void                    application_default_update              (Application<Bindings>* application);
    template<typename Bindings> void                    application_default_render              (Application<Bindings>* application);
    template<typename Bindings> bool                    application_should_quit                 (Application<Bindings>* application);
}

#endif
