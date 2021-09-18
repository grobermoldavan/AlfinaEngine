
#include <cstring>

#include "platform_window_win32.h"
#include "platform_input_win32.h"

namespace al
{
    void platform_input_construct(PlatformInput* input)
    {
        std::memset(input, 0, sizeof(PlatformInput));
        // Get active window from the current thread
        HWND windowHandle = ::GetActiveWindow();
        PlatformWindow* window = reinterpret_cast<PlatformWindow*>(::GetWindowLongPtr(windowHandle, GWLP_USERDATA));
        input->window = window;
    }

    void platform_input_destruct(PlatformInput* input)
    {

    }

    void platform_input_update(PlatformInput* input)
    {
        std::memcpy(input, &input->window->input, sizeof(decltype(input->keyboard)) + sizeof(decltype(input->mouse)));
    }

    bool platform_input_is_mouse_input_active(PlatformInput* input, MouseInput flag)
    {
        return input->mouse.buttons & (1 << (static_cast<u32>(flag)));
    }

    bool platform_input_is_keyboard_input_active(PlatformInput* input, KeyboardInput flag)
    {
        u64 u64flag = static_cast<u64>(flag);
        u64 id = u64flag / 64ULL;
        return input->keyboard.buttons[id] & (1ULL << (u64flag - id * 64ULL));
    }
}
