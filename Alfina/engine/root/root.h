#ifndef __ALFINA_ROOT_H__
#define __ALFINA_ROOT_H__

#include <functional>

#define AL_WIN_32

#include "engine/engine_utilities/error_info.h"

#include "engine/memory/base_memory_manager.h"

#include "engine/window/base_application_window.h"
#ifdef AL_WIN_32
#include "engine/window/win32/windows_application_window.h"
#else
#error Unsupported plaform
#endif

#include "engine/file_system/base_file_system.h"
#ifdef AL_WIN_32
#include "engine/file_system/win32/windows_file_system.h"
#else
#error Unsupported plaform
#endif

#include "engine/renderer/base_renderer.h"
#ifdef AL_WIN_32
#include "engine/renderer/win32/windows_opengl_renderer.h"
#else
#error Unsupported plaform
#endif

#include "engine/sound_system/base_sound_system.h"
#ifdef AL_WIN_32
#include "engine/sound_system/win32/windows_sound_system.h"
#else
#error Unsupported plaform
#endif

namespace al::engine
{
    struct RootInitInfo
    {
        WindowProperties windowProperties; // Properties of the window
        SoundParameters soundParameters;   // Properties of the sound
        size_t memoryBytes;                // Amound of memory to be allocated for the root
    };

    class Root
    {
    public:
        using MainAllocator = std::remove_pointer<std::invoke_result<decltype(&MemoryManager::get_main_allocator), MemoryManager*>::type>::type;

        Root(const RootInitInfo &initInfo);
        ~Root();

        ErrorInfo               get_construction_result()                       const   noexcept;
        MemoryManager *         get_memory_manager()                                    noexcept;
        ApplicationWindow *     get_window()                                            noexcept;
        FileSystem *            get_file_system()                                       noexcept;
        Renderer *              get_renderer()                                          noexcept;
        SoundSystem *           get_sound_system()                                      noexcept;
        void                    get_input(ApplicationWindowInput *inputBuffer)  const   noexcept;

    private:
        MemoryManager       memory;
        ApplicationWindow * window;
        FileSystem *        fileSystem;
        Renderer *          renderer;
        SoundSystem *       sound;
        ErrorInfo           constructionResult;

        template <typename T, typename U, typename... Args>
        ErrorInfo system_create(U **systemPtr, std::function<uint8_t *(size_t sizeBytes)> allocate, Args... args) noexcept;

        template <typename T, typename U>
        ErrorInfo system_destroy(U *systemPtr, std::function<void(uint8_t *ptr)> deallocate) noexcept;
    };

} // namespace al::engine

#include "root.cpp"

#endif
