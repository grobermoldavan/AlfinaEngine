#ifndef AL_APPLICATION_H
#define AL_APPLICATION_H

#include <type_traits>  // for std::false_type, std::true_type

#include "engine/memory/memory.h"
#include "engine/platform/platform.h"
#include "engine/render/renderer.h"

#define HAS_CHECK_DECLARATION(functionName)                                                     \
    template <typename T, typename = int>                                                       \
    struct __has##functionName : std::false_type { };                                           \
    template <typename T>                                                                       \
    struct __has##functionName <T, decltype((void) T::functionName, 0)> : std::true_type { };

#define HAS_CHECK(bindings, functionName) __has##functionName<bindings>::value

namespace al
{
    HAS_CHECK_DECLARATION(create);
    HAS_CHECK_DECLARATION(destroy);
    HAS_CHECK_DECLARATION(update);
    HAS_CHECK_DECLARATION(render);
    HAS_CHECK_DECLARATION(should_quit);
    HAS_CHECK_DECLARATION(handle_window_resize);
    HAS_CHECK_DECLARATION(get_creation_data);

    struct CommandLineArgs
    {
        int argc;
        char** argv;
    };

    template<typename Bindings>
    struct Application
    {
        StackAllocator          stack;
        PoolAllocator           pool;
        PlatformWindow          window;
        PlatformInput           input;
        Renderer<vulkan::RendererBackend> renderer;
        Bindings                bindings;
    };

    struct ApplicationCreationData
    {
        PlatformWindowInitData windowInitData;
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
