#ifndef AL_APPLICATION_H
#define AL_APPLICATION_H

#include "engine/memory/memory.h"
#include "engine/platform/platform.h"
#include "engine/render/renderer.h"
#include "engine/utilities/utilities.h"
#include "engine/thread_local_globals/thread_local_globals.h"

namespace al
{
    AL_HAS_CHECK_DECLARATION(should_quit);
    AL_HAS_CHECK_DECLARATION(get_creation_data);
    AL_HAS_CHECK_DECLARATION(subsystems);

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

        PlatformWindow      window;
        PlatformInput       input;
        Renderer            renderer;
        Logger*             logger;
        ApplicationGlobals  globals;

        Bindings bindings;
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
