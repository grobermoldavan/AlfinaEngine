
#include <stdio.h>
#include <functional>

#include "engine/asserts/asserts.h"

#ifdef AL_WIN_32
#define OS_SYSTEM_TYPE(base) Win32##base
#else
#error Unsupported plaform
#endif

namespace al::engine
{
    Root::Root(const RootInitInfo &initInfo)
        : window    {nullptr}
        , fileSystem{nullptr}
        , renderer  {nullptr}
        , sound     {nullptr}
        , memory    {initInfo.memoryBytes}
    {
        // @TODO : log errors instead of asserts

        // Initialize memory
        constructionResult = memory.get_construction_result();
        al_assert(constructionResult);

        // @NOTE : std::ref helps avoid heap allocations while creating std::function
        auto alloc = [&](size_t bytes) -> uint8_t * { return memory.get_stack()->allocate(bytes); };
        auto refAlloc = std::ref(alloc);

        // Create application window
        constructionResult = system_create<OS_SYSTEM_TYPE(ApplicationWindow)>(&window, refAlloc, initInfo.windowProperties, memory.get_main_allocator());
        al_assert(constructionResult);

        // Create file system
        constructionResult = system_create<OS_SYSTEM_TYPE(FileSystem)>(&fileSystem, refAlloc, memory.get_main_allocator());
        al_assert(constructionResult);

        { // test file read
            uint8_t *top = memory.get_stack()->get_current_top();
            FileHandle fh;
            fileSystem->read_file("Assets\\test.txt", &fh);
            if (fh.get_data())
                printf("Data is : %s\n", fh.get_data());
            else
                printf("Unable to read file.");
            memory.get_stack()->free_to_pointer(top);
        }

        // Create renderer
        // @TODO : make Win32Renderer instead of Win32glRenderer
        constructionResult = system_create<OS_SYSTEM_TYPE(glRenderer)>(&renderer, refAlloc, window, memory.get_main_allocator());
        al_assert(constructionResult);

        // Create sound system
        constructionResult = system_create<OS_SYSTEM_TYPE(SoundSystem)>(&sound, refAlloc, window, fileSystem, initInfo.soundParameters, memory.get_main_allocator());
        al_assert(constructionResult);
    }

    Root::~Root()
    {
        // @TODO : log errors instead of asserts

        ErrorInfo error;

        auto dealloc = [&](uint8_t *ptr) -> void { return memory.get_stack()->free_to_pointer(ptr); };
        auto refDealloc = std::ref(dealloc);

        // Destroy sound system
        if (sound)
        {
            error = system_destroy<OS_SYSTEM_TYPE(SoundSystem)>(sound, refDealloc);
            al_assert(error);
        }

        // Destroy renderer
        if (renderer)
        {
            error = system_destroy<OS_SYSTEM_TYPE(glRenderer)>(renderer, refDealloc);
            al_assert(error);
        }

        // Destroy file system
        if (fileSystem)
        {
            error = system_destroy<OS_SYSTEM_TYPE(FileSystem)>(fileSystem, refDealloc);
            al_assert(error);
        }

        // Destroy application window
        if (window)
        {
            error = system_destroy<OS_SYSTEM_TYPE(ApplicationWindow)>(window, refDealloc);
            al_assert(error);
        }
    }

    ErrorInfo Root::get_construction_result() const noexcept
    {
        return constructionResult;
    }

    MemoryManager *Root::get_memory_manager() noexcept
    {
        return &memory;
    }

    ApplicationWindow *Root::get_window() noexcept
    {
        return window;
    }

    FileSystem *Root::get_file_system() noexcept
    {
        return fileSystem;
    }

    Renderer *Root::get_renderer() noexcept
    {
        return renderer;
    }

    SoundSystem *Root::get_sound_system() noexcept
    {
        return sound;
    }

    void Root::get_input(ApplicationWindowInput *inputBuffer) const noexcept
    {
        window->get_input(inputBuffer);
    }

    template <typename T, typename U, typename... Args>
    ErrorInfo Root::system_create(U **systemPtr, std::function<uint8_t *(size_t sizeBytes)> allocate, Args... args) noexcept
    {
        if (!systemPtr)
        {
            return {ErrorInfo::Code::INCORRECT_INPUT_DATA, ErrorInfo::ERROR_MESSAGES[ErrorInfo::ErrorMessageCode::NULL_PTR_PROVIDED]};
        }

        // Allocate memory and construct object
        T *system = reinterpret_cast<T *>(allocate(sizeof(T)));
        if (!system)
        {
            return {ErrorInfo::Code::ALLOCATION_ERROR, ErrorInfo::ERROR_MESSAGES[ErrorInfo::ErrorMessageCode::UNABLE_TO_ALLOCATE_MEMORY]};
        }

        system = ::new (system) T(args...);
        *systemPtr = system;

        return {ErrorInfo::Code::ALL_FINE, ErrorInfo::ERROR_MESSAGES[ErrorInfo::ErrorMessageCode::ALL_FINE]};
    }

    template <typename T, typename U>
    ErrorInfo Root::system_destroy(U *systemPtr, std::function<void(uint8_t *ptr)> deallocate) noexcept
    {
        if (!systemPtr)
        {
            return {ErrorInfo::Code::INCORRECT_INPUT_DATA, ErrorInfo::ERROR_MESSAGES[ErrorInfo::ErrorMessageCode::NULL_PTR_PROVIDED]};
        }

        T *osSystemPtr = static_cast<T *>(systemPtr);

        // Destruct and deallocate system
        osSystemPtr->~T();
        deallocate(reinterpret_cast<uint8_t *>(osSystemPtr));

        return {ErrorInfo::Code::ALL_FINE, ErrorInfo::ERROR_MESSAGES[ErrorInfo::ErrorMessageCode::ALL_FINE]};
    }
    
} // namespace al::engine
