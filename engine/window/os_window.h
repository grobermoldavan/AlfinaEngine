#ifndef OS_WINDOW_H
#define OS_WINDOW_H

#include <string_view>
#include <cstdint>

#include "utilities/flags.h"

namespace al::engine
{
    class OsWindow;
    struct OsWindowParams;

    [[nodiscard]] OsWindow* create_window(const OsWindowParams& params) noexcept;   // @NOTE : this function is defined in os header
    void destroy_window(OsWindow* window) noexcept;                                 // @NOTE : this function is defined in os header

    struct OsWindowInput
    {
        enum class MouseInputFlags : uint32_t
        {
            LMB_PRESSED,
            RMB_PRESSED,
            MMB_PRESSED,
            __end
        };

        enum class KeyboardInputFlags : uint32_t
        {
            NONE,
            Q, W, E, R, T, Y, U, I, O, P, A, S, D, F, G, H, J, K, L, Z, X, C, V, B, N, M,
            L_BRACKET, R_BRACKET, SEMICOLON, APOSTROPHE, BACKSLASH, COMMA, PERIOD, SLASH,
            TILDA, NUM_1, NUM_2, NUM_3, NUM_4, NUM_5, NUM_6, NUM_7, NUM_8, NUM_9, NUM_0, MINUS, EQUALS,
            TAB, CAPS_LOCK, ENTER, BACKSPACE, SPACE,
            L_SHIFT, R_SHIFT, SHIFT,
            L_CTRL, R_CTRL, CTRL,
            L_ALT, R_ALT, ALT,
            ESCAPE, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
            ARROW_UP, ARROW_DOWN, ARROW_LEFT, ARROW_RIGHT, __end
        };

        static constexpr const char* KEYBOARD_FLAGS_TO_STRING[] =
        {
            "NONE",
            "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "A", "S", "D", "F", "G", "H", "J", "K", "L", "Z", "X", "C", "V", "B", "N", "M",
            "L_BRACKET", "R_BRACKET", "SEMICOLON", "APOSTROPHE", "BACKSLASH", "COMMA", "PERIOD", "SLASH",
            "TILDA", "NUM_1", "NUM_2", "NUM_3", "NUM_4", "NUM_5", "NUM_6", "NUM_7", "NUM_8", "NUM_9", "NUM_0", "MINUS", "EQUALS",
            "TAB", "CAPS_LOCK", "ENTER", "BACKSPACE", "SPACE",
            "L_SHIFT", "R_SHIFT", "SHIFT",
            "L_CTRL", "R_CTRL", "CTRL",
            "L_ALT", "R_ALT", "ALT",
            "ESCAPE", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12",
            "ARROW_UP", "ARROW_DOWN", "ARROW_LEFT", "ARROW_RIGHT"
        };

        static constexpr const char* MOUSE_FLAGS_TO_STRING[] =
        {
            "LMB", "RMB", "MMB"
        };

        struct
        {
            Flags128 buttons;
        } keyboard { };

        struct {
            Flags32 buttons;
            int32_t x;
            int32_t y;
            int32_t wheel;
        } mouse { };
    };

    struct OsWindowParams
    {
        std::string_view name = "Alfina Engine";
        std::size_t width = 1024;
        std::size_t height = 768;
    };

    class OsWindow
    {
    public:
        OsWindow(const OsWindowParams& params) noexcept;
        virtual ~OsWindow() noexcept = default;

        virtual void process() noexcept = 0;
        virtual void quit() noexcept = 0;
        virtual bool is_quit() noexcept = 0;
        virtual OsWindowInput get_input() noexcept = 0;

    protected:
        OsWindowParams params;
    };

    OsWindow::OsWindow(const OsWindowParams& params) noexcept
        : params{ params }
    { }
}

#ifdef AL_PLATFORM_WIN32
#   include "os_window_win32.h"
#else
#   error Unsupported platform
#endif

#endif
