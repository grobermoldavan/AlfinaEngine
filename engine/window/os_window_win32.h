#ifndef AL_OS_WINDOW_WIN32_H
#define AL_OS_WINDOW_WIN32_H

#include <Windows.h>
#include <Windowsx.h>

#include "os_window.h"
#include "engine/debug/debug.h"
#include "engine/memory/memory_manager.h"

namespace al::engine
{
    LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept;

    class OsWindowWin32 : public OsWindow
    {
    public:
        OsWindowWin32(const OsWindowParams& params) noexcept;
        ~OsWindowWin32() noexcept = default;

        HWND get_handle() noexcept { return handle; }

        virtual void quit() noexcept override;
        virtual void process() noexcept override;
        virtual bool is_quit() noexcept override;
        virtual OsWindowInput get_input() noexcept override;

    private:
        OsWindowInput input;
        HWND handle;
        bool isQuit;

        friend bool process_mouse_input(OsWindowWin32*, UINT, WPARAM, LPARAM) noexcept;
        friend bool process_keyboard_input(OsWindowWin32*, UINT, WPARAM, LPARAM) noexcept;
    };

    OsWindowWin32::OsWindowWin32(const OsWindowParams& params) noexcept
        : OsWindow{ params }
        , handle{ nullptr }
        , isQuit{ false }
        , input{ }
    {
        HMODULE moduleHandle = ::GetModuleHandle(NULL);
        
        WNDCLASSA wc = {};
        wc.lpfnWndProc      = WindowProc;
        wc.hInstance        = moduleHandle;
        wc.lpszClassName    = params.name.data();
        wc.hCursor          = ::LoadCursor(NULL, IDC_ARROW);

        const bool isClassRegistered = ::RegisterClassA(&wc);
        al_assert(isClassRegistered);

        constexpr DWORD windowedStyle = 
            WS_OVERLAPPED   |           // Window has title and borders
            WS_SYSMENU      |           // Add sys menu (close button)
            WS_MINIMIZEBOX  ;           // Add minimize button
        
        constexpr DWORD fullscreenStyle = WS_POPUP;

        const bool isFullscreen = false;

        handle = ::CreateWindowExA(
            0,                                              // Optional window styles.
            params.name.data(),                             // Window class
            params.name.data(),                             // Window text
            isFullscreen ? fullscreenStyle : windowedStyle, // Window styles
            CW_USEDEFAULT,                                  // Position X
            CW_USEDEFAULT,                                  // Position Y
            params.width,                                   // Size X
            params.height,                                  // Size Y
            NULL,                                           // Parent window    
            NULL,                                           // Menu
            moduleHandle,                                   // Instance handle
            NULL                                            // Additional application data
        );

        al_assert(handle);

        ::SetWindowLongPtr(handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

        ::ShowWindow(handle, isFullscreen ? SW_MAXIMIZE : SW_SHOW);
        ::UpdateWindow(handle);

        process();
    }

    void OsWindowWin32::process() noexcept
    {
        al_assert(!is_quit());

        MSG msg = {};
        while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) > 0)
        {
            if (msg.message == WM_QUIT)
            {
                isQuit = true;
                break;
            }

            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
    }

    void OsWindowWin32::quit() noexcept
    {
        ::PostQuitMessage(0);
    }

    bool OsWindowWin32::is_quit() noexcept
    {
        return isQuit;
    }

    OsWindowInput OsWindowWin32::get_input() noexcept
    {
        return input;
    }

    LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept
    {
        OsWindowWin32* window = (OsWindowWin32*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

        if (process_mouse_input(window, uMsg, wParam, lParam))
        {
            return 0;
        }

        if (process_keyboard_input(window, uMsg, wParam, lParam))
        {
            return 0;
        }

        switch (uMsg)
        {
        case WM_DESTROY:
            ::PostQuitMessage(0);
            return 0;
        case WM_CLOSE:
            ::PostQuitMessage(0);
            return 0;
        }

        return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    bool process_mouse_input(OsWindowWin32* window, UINT message, WPARAM wParam, LPARAM lParam) noexcept
    {
        #define LOGMSG(msg, ...)

        switch (message)
        {
            case WM_LBUTTONDOWN:
            {
                window->input.mouse.buttons.set_flag(static_cast<uint32_t>(OsWindowInput::MouseInputFlags::LMB_PRESSED));
                LOGMSG(WM_LBUTTONDOWN)
                break;
            }
            case WM_LBUTTONUP:
            {
                window->input.mouse.buttons.clear_flag(static_cast<uint32_t>(OsWindowInput::MouseInputFlags::LMB_PRESSED));
                LOGMSG(WM_LBUTTONUP)
                break;
            }
            case WM_MBUTTONDOWN:
            {
                window->input.mouse.buttons.set_flag(static_cast<uint32_t>(OsWindowInput::MouseInputFlags::MMB_PRESSED));
                LOGMSG(WM_MBUTTONDOWN)
                break;
            }
            case WM_MBUTTONUP:
            {
                window->input.mouse.buttons.clear_flag(static_cast<uint32_t>(OsWindowInput::MouseInputFlags::MMB_PRESSED));
                LOGMSG(WM_MBUTTONUP)
                break;
            }
            case WM_RBUTTONDOWN:
            {
                window->input.mouse.buttons.set_flag(static_cast<uint32_t>(OsWindowInput::MouseInputFlags::RMB_PRESSED));
                LOGMSG(WM_RBUTTONDOWN)
                break;
            }
            case WM_RBUTTONUP:
            {
                window->input.mouse.buttons.clear_flag(static_cast<uint32_t>(OsWindowInput::MouseInputFlags::RMB_PRESSED));
                LOGMSG(WM_RBUTTONUP)
                break;
            }
            case WM_XBUTTONDOWN:
            {
                LOGMSG(WM_XBUTTONDOWN)
                break;
            }
            case WM_XBUTTONUP:
            {
                LOGMSG(WM_XBUTTONUP)
                break;
            }
            case WM_MOUSEMOVE:
            {
                break;
            }
            case WM_MOUSEWHEEL:
            {
                window->input.mouse.wheel += GET_WHEEL_DELTA_WPARAM(wParam);
                LOGMSG(WM_MOUSEWHEEL, " ", GET_WHEEL_DELTA_WPARAM(wParam))
                break;
            }
            default:
            {
                return false;
            }
        }

        window->input.mouse.x = GET_X_LPARAM(lParam);
        window->input.mouse.y = GET_Y_LPARAM(lParam);

        return true;

        #undef LOGMSG
    }

    bool process_keyboard_input(OsWindowWin32* window, UINT message, WPARAM wParam, LPARAM lParam) noexcept
    {
        #define k(key) static_cast<uint32_t>(OsWindowInput::KeyboardInputFlags::key)

        static uint32_t VK_CODE_TO_KEYBOARD_FLAGS[] =
        {
            k(NONE),        // 0x00 - none
            k(NONE),        // 0x01 - lmb
            k(NONE),        // 0x02 - rmb
            k(NONE),        // 0x03 - control-break processing
            k(NONE),        // 0x04 - mmb
            k(NONE),        // 0x05 - x1 mouse button
            k(NONE),        // 0x06 - x2 mouse button
            k(NONE),        // 0x07 - undefined
            k(BACKSPACE),   // 0x08 - backspace
            k(TAB),         // 0x09 - tab
            k(NONE),        // 0x0A - reserved
            k(NONE),        // 0x0B - reserved
            k(NONE),        // 0x0C - clear key
            k(ENTER),       // 0x0D - enter
            k(NONE),        // 0x0E - undefined
            k(NONE),        // 0x0F - undefined
            k(SHIFT),       // 0x10    - shift
            k(CTRL),        // 0x11 - ctrl
            k(ALT),         // 0x12 - alt
            k(NONE),        // 0x13 - pause
            k(CAPS_LOCK),   // 0x14 - caps lock
            k(NONE),        // 0x15 - IME Kana mode / Handuel mode
            k(NONE),        // 0x16 - IME On
            k(NONE),        // 0x17 - IME Junja mode
            k(NONE),        // 0x18 - IME Final mode
            k(NONE),        // 0x19 - IME Hanja mode / Kanji mode
            k(NONE),        // 0x1A - IME Off
            k(ESCAPE),      // 0x1B - ESC key
            k(NONE),        // 0x1C - IME convert
            k(NONE),        // 0x1D - IME nonconvert
            k(NONE),        // 0x1E - IME accept
            k(NONE),        // 0x1F - IME mode change request
            k(SPACE),       // 0x20 - spacebar
            k(NONE),        // 0x21 - page up
            k(NONE),        // 0x22 - page down
            k(NONE),        // 0x23 - end
            k(NONE),        // 0x24 - home
            k(ARROW_LEFT),  // 0x25 - left arrow
            k(ARROW_UP),    // 0x26 - up arrow
            k(ARROW_RIGHT), // 0x27 - right arrow
            k(ARROW_DOWN),  // 0x28 - down arrow
            k(NONE),        // 0x29 - select
            k(NONE),        // 0x2A - print
            k(NONE),        // 0x2B - execute
            k(NONE),        // 0x2C - print screen
            k(NONE),        // 0x2D - ins
            k(NONE),        // 0x2E - del
            k(NONE),        // 0x2F - help
            k(NUM_0),       // 0x30 - 0
            k(NUM_1),       // 0x31 - 1
            k(NUM_2),       // 0x32 - 2
            k(NUM_3),       // 0x33 - 3
            k(NUM_4),       // 0x34 - 4
            k(NUM_5),       // 0x35 - 5
            k(NUM_6),       // 0x36 - 6
            k(NUM_7),       // 0x37 - 7
            k(NUM_8),       // 0x38 - 8
            k(NUM_9),       // 0x39 - 9
            k(NONE),        // 0x3A - undefined
            k(NONE),        // 0x3B - undefined
            k(NONE),        // 0x3C - undefined
            k(NONE),        // 0x3D - undefined
            k(NONE),        // 0x3E - undefined
            k(NONE),        // 0x3F - undefined
            k(NONE),        // 0x40 - undefined
            k(A),           // 0x41 - A
            k(B),           // 0x42 - B
            k(C),           // 0x43 - C
            k(D),           // 0x44 - D
            k(E),           // 0x45 - E
            k(F),           // 0x46 - F
            k(G),           // 0x47 - G
            k(H),           // 0x48 - H
            k(I),           // 0x49 - I
            k(G),           // 0x4A - G
            k(K),           // 0x4B - K
            k(L),           // 0x4C - L
            k(M),           // 0x4D - M
            k(N),           // 0x4E - N
            k(O),           // 0x4F - O
            k(P),           // 0x50 - P
            k(Q),           // 0x51 - Q
            k(R),           // 0x52 - R
            k(S),           // 0x53 - S
            k(T),           // 0x54 - T
            k(U),           // 0x55 - U
            k(V),           // 0x56 - V
            k(W),           // 0x57 - W
            k(X),           // 0x58 - X
            k(Y),           // 0x59 - Y
            k(Z),           // 0x5A - Z
            k(NONE),        // 0x5B - left windows
            k(NONE),        // 0x5C - right windows
            k(NONE),        // 0x5D - applications
            k(NONE),        // 0x5E - reserved
            k(NONE),        // 0x5F - computer sleep
            k(NONE),        // 0x60 - numeric keypad 0
            k(NONE),        // 0x61 - numeric keypad 1
            k(NONE),        // 0x62 - numeric keypad 2
            k(NONE),        // 0x63 - numeric keypad 3
            k(NONE),        // 0x64 - numeric keypad 4
            k(NONE),        // 0x65 - numeric keypad 5
            k(NONE),        // 0x66 - numeric keypad 6
            k(NONE),        // 0x67 - numeric keypad 7
            k(NONE),        // 0x68 - numeric keypad 8
            k(NONE),        // 0x69 - numeric keypad 9
            k(NONE),        // 0x6A - numeric keypad multiply
            k(NONE),        // 0x6B - numeric keypad add
            k(NONE),        // 0x6C - numeric keypad separator
            k(NONE),        // 0x6D - numeric keypad subtract
            k(NONE),        // 0x6E - numeric keypad decimal
            k(NONE),        // 0x6F - numeric keypad divide
            k(F1),          // 0x70 - F1
            k(F2),          // 0x71 - F2
            k(F3),          // 0x72 - F3
            k(F4),          // 0x73 - F4
            k(F5),          // 0x74 - F5
            k(F6),          // 0x75 - F6
            k(F7),          // 0x76 - F7
            k(F8),          // 0x77 - F8
            k(F9),          // 0x78 - F9
            k(F10),         // 0x79 - F10
            k(F11),         // 0x7A - F11
            k(F12),         // 0x7B - F12
            k(NONE),        // 0x7C - F13
            k(NONE),        // 0x7D - F14
            k(NONE),        // 0x7E - F15
            k(NONE),        // 0x7F - F16
            k(NONE),        // 0x80 - F17
            k(NONE),        // 0x81 - F18
            k(NONE),        // 0x82 - F19
            k(NONE),        // 0x83 - F20
            k(NONE),        // 0x84 - F21
            k(NONE),        // 0x85 - F22
            k(NONE),        // 0x86 - F23
            k(NONE),        // 0x87 - F24
            k(NONE),        // 0x88 - unassigned
            k(NONE),        // 0x89 - unassigned
            k(NONE),        // 0x8A - unassigned
            k(NONE),        // 0x8B - unassigned
            k(NONE),        // 0x8C - unassigned
            k(NONE),        // 0x8D - unassigned
            k(NONE),        // 0x8E - unassigned
            k(NONE),        // 0x8F - unassigned
            k(NONE),        // 0x90 - num lock
            k(NONE),        // 0x91 - scroll lock
            k(NONE),        // 0x92 - OEM specific
            k(NONE),        // 0x93 - OEM specific
            k(NONE),        // 0x94 - OEM specific
            k(NONE),        // 0x95 - OEM specific
            k(NONE),        // 0x96 - OEM specific
            k(NONE),        // 0x97 - unassigned
            k(NONE),        // 0x98 - unassigned
            k(NONE),        // 0x99 - unassigned
            k(NONE),        // 0x9A - unassigned
            k(NONE),        // 0x9B - unassigned
            k(NONE),        // 0x9C - unassigned
            k(NONE),        // 0x9D - unassigned
            k(NONE),        // 0x9E - unassigned
            k(NONE),        // 0x9F - unassigned
            k(L_SHIFT),     // 0xA0 - left shift
            k(R_SHIFT),     // 0xA1 - right shift
            k(L_CTRL),      // 0xA2 - left ctrl
            k(R_CTRL),      // 0xA3 - right ctrl
            k(NONE),        // 0xA4 - left menu
            k(NONE),        // 0xA5 - right menu
            k(NONE),        // 0xA6 - browser back
            k(NONE),        // 0xA7 - browser forward
            k(NONE),        // 0xA8 - browser refresh
            k(NONE),        // 0xA9 - browser stop
            k(NONE),        // 0xAA - browser search
            k(NONE),        // 0xAB - browser favorites
            k(NONE),        // 0xAC - browser start and home
            k(NONE),        // 0xAD - volume mute
            k(NONE),        // 0xAE - volume down
            k(NONE),        // 0xAF - volume up
            k(NONE),        // 0xB0 - next track
            k(NONE),        // 0xB1 - previous track
            k(NONE),        // 0xB2 - stop media
            k(NONE),        // 0xB3 - play / pause media
            k(NONE),        // 0xB4 - start mail
            k(NONE),        // 0xB5 - select media
            k(NONE),        // 0xB6 - start app 1
            k(NONE),        // 0xB7 - start app 2
            k(NONE),        // 0xB8 - reserved
            k(NONE),        // 0xB9 - reserved
            k(SEMICOLON),   // 0xBA - ';:' for US keyboard
            k(EQUALS),      // 0xBB - plus / equals
            k(COMMA),       // 0xBC - comma
            k(MINUS),       // 0xBD - minus
            k(PERIOD),      // 0xBE - period
            k(SLASH),       // 0xBF - '/?' for US keyboard
            k(TILDA),       // 0xC0 - '~' for US keyboard
            k(NONE),        // 0xC1 - reserved
            k(NONE),        // 0xC2 - reserved
            k(NONE),        // 0xC3 - reserved
            k(NONE),        // 0xC4 - reserved
            k(NONE),        // 0xC5 - reserved
            k(NONE),        // 0xC6 - reserved
            k(NONE),        // 0xC7 - reserved
            k(NONE),        // 0xC8 - reserved
            k(NONE),        // 0xC9 - reserved
            k(NONE),        // 0xCA - reserved
            k(NONE),        // 0xCB - reserved
            k(NONE),        // 0xCC - reserved
            k(NONE),        // 0xCD - reserved
            k(NONE),        // 0xCE - reserved
            k(NONE),        // 0xCF - reserved
            k(NONE),        // 0xD0 - reserved
            k(NONE),        // 0xD1 - reserved
            k(NONE),        // 0xD2 - reserved
            k(NONE),        // 0xD3 - reserved
            k(NONE),        // 0xD4 - reserved
            k(NONE),        // 0xD5 - reserved
            k(NONE),        // 0xD6 - reserved
            k(NONE),        // 0xD7 - reserved
            k(NONE),        // 0xD8 - unassigned
            k(NONE),        // 0xD9 - unassigned
            k(NONE),        // 0xDA - unassigned
            k(L_BRACKET),   // 0xDB - '[{' for US keyboard
            k(BACKSLASH),   // 0xDC - '\|' for US keyboard
            k(R_BRACKET),   // 0xDD - ']}' for US keyboard
            k(APOSTROPHE),  // 0xDE - ''"' for US keyboard
            k(NONE),        // 0xDF - misc char
            k(NONE),        // 0xE0 - reserved
            k(NONE),        // 0xE1 - OEM specific
            k(NONE),        // 0xE2 - VK_OEM_102
            k(NONE),        // 0xE3 - OEM specific
            k(NONE),        // 0xE4 - OEM specific
            k(NONE),        // 0xE5 - IME process
            k(NONE),        // 0xE6 - OEM specific
            k(NONE),        // 0xE7 - VK_PACKET
            k(NONE),        // 0xE8 - unassigned
            k(NONE),        // 0xE9 - OEM specific
            k(NONE),        // 0xEA - OEM specific
            k(NONE),        // 0xEB - OEM specific
            k(NONE),        // 0xEC - OEM specific
            k(NONE),        // 0xED - OEM specific
            k(NONE),        // 0xEE - OEM specific
            k(NONE),        // 0xEF - OEM specific
            k(NONE),        // 0xF0 - OEM specific
            k(NONE),        // 0xF1 - OEM specific
            k(NONE),        // 0xF2 - OEM specific
            k(NONE),        // 0xF3 - OEM specific
            k(NONE),        // 0xF4 - OEM specific
            k(NONE),        // 0xF5 - OEM specific
            k(NONE),        // 0xF6 - Attn
            k(NONE),        // 0xF7 - CrSel
            k(NONE),        // 0xF8 - ExSel
            k(NONE),        // 0xF9 - Erase EOF
            k(NONE),        // 0xFA - Play
            k(NONE),        // 0xFB - Zoom
            k(NONE),        // 0xFC - reserved
            k(NONE),        // 0xFD - PA1
            k(NONE)         // 0xFE - clear
        };

        #undef k

        switch (message)
        {
            case WM_KEYDOWN:
            {
                window->input.keyboard.buttons.set_flag(VK_CODE_TO_KEYBOARD_FLAGS[wParam]);
                break;
            }
            case WM_KEYUP:
            {
                window->input.keyboard.buttons.clear_flag(VK_CODE_TO_KEYBOARD_FLAGS[wParam]);
                break;
            }
            default:
            {
                return false;
            }
        }

        return true;
    }

    [[nodsicard]] OsWindow* create_window(const OsWindowParams& params) noexcept
    {
        OsWindow* window = MemoryManager::get()->get_stack()->allocate_as<OsWindowWin32>();
        ::new(window) OsWindowWin32({ });
        return window;
    }

    void destroy_window(OsWindow* window) noexcept
    {
        window->~OsWindow();
        MemoryManager::get()->get_stack()->deallocate(reinterpret_cast<std::byte*>(window), sizeof(OsWindowWin32));
    }
}

#endif
