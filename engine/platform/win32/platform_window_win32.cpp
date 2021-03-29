
#include <cstring>

#include "engine/platform/win32/platform_window_win32.h"

namespace al
{
    static LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static bool process_mouse_input(PlatformWindow* window, UINT message, WPARAM wParam, LPARAM lParam);
    static bool process_keyboard_input(PlatformWindow* window, UINT message, WPARAM wParam, LPARAM lParam);

    void platform_window_construct(PlatformWindow* window, const PlatformWindowInitData& initData)
    {
        std::memset(window, 0, sizeof(PlatformWindow));

        HMODULE moduleHandle = ::GetModuleHandle(NULL);
        
        WNDCLASSA wc = {};
        wc.lpfnWndProc      = WindowProc;
        wc.hInstance        = moduleHandle;
        wc.lpszClassName    = initData.name;
        wc.hCursor          = ::LoadCursor(NULL, IDC_ARROW);

        const bool isClassRegistered = ::RegisterClassA(&wc);
        // al_assert(isClassRegistered);

        constexpr DWORD windowedStyleNoResize = 
            WS_OVERLAPPED   |           // Window has title and borders
            WS_SYSMENU      |           // Add sys menu (close button)
            WS_MINIMIZEBOX  ;           // Add minimize button

        constexpr DWORD windowedStyleResize = 
            WS_OVERLAPPED   |           // Window has title and borders
            WS_SYSMENU      |           // Add sys menu (close button)
            WS_MINIMIZEBOX  |           // Add minimize button
            WS_MAXIMIZEBOX  |           // Add maximize button
            WS_SIZEBOX      ;           // Add ability to resize window by dragging it's frame
        
        constexpr DWORD fullscreenStyle = WS_POPUP;

        DWORD activeStyle = initData.isFullscreen ? fullscreenStyle : (initData.isResizable ? windowedStyleResize : windowedStyleNoResize);

        // @TODO : safe cast from u32 to int
        int targetWidth = static_cast<int>(initData.width);
        int targetHeight = static_cast<int>(initData.height);

        window->handle = ::CreateWindowExA(
            0,                  // Optional window styles.
            initData.name,      // Window class
            initData.name,      // Window text
            activeStyle,        // Window styles
            CW_USEDEFAULT,      // Position X
            CW_USEDEFAULT,      // Position Y
            targetWidth,        // Size X
            targetHeight,       // Size Y
            NULL,               // Parent window    
            NULL,               // Menu
            moduleHandle,       // Instance handle
            NULL                // Additional application data
        );
        // al_assert(window->handle);
        if (!initData.isFullscreen)
        {
            // Set correct size for a client rect
            // Retrieve currecnt client rect size and calculate difference with target size
            RECT clientRect;
            bool getClientRectResult = ::GetClientRect(window->handle, &clientRect);
            // al_assert(getClientRectResult);
            int widthDiff = targetWidth - (clientRect.right - clientRect.left);
            int heightDiff = targetHeight - (clientRect.bottom - clientRect.top);
            // Retrieve currecnt window rect size (this includes borders!)
            RECT windowRect;
            bool getWindowRectResult = ::GetWindowRect(window->handle, &windowRect);
            // al_assert(getWindowRectResult);
            int windowWidth = windowRect.right - windowRect.left;
            int windowHeight = windowRect.bottom - windowRect.top;
            // Set new window size based on calculated diff
            bool setWindowPosResult = ::SetWindowPos(window->handle, NULL, windowRect.left, windowRect.top, windowWidth + widthDiff, windowHeight + heightDiff, SWP_NOMOVE | SWP_NOZORDER);
            // al_assert(setWindowPosResult);
            // Retrieve resulting size
            getClientRectResult = ::GetClientRect(window->handle, &clientRect);
            // al_assert(getClientRectResult);
            window->width = static_cast<u32>(clientRect.right - clientRect.left);
            window->height = static_cast<u32>(clientRect.bottom - clientRect.top);
        }
        ::SetWindowLongPtr(window->handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
        ::ShowWindow(window->handle, initData.isFullscreen ? SW_MAXIMIZE : SW_SHOW);
        ::UpdateWindow(window->handle);
        platform_window_process(window);
    }

    void platform_window_destruct(PlatformWindow* window)
    {
        ::DestroyWindow(window->handle);
        platform_window_process(window);
    }

    void platform_window_process(PlatformWindow* window)
    {
        MSG msg = { };
        while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
    }

    bool platform_window_is_close_button_pressed(PlatformWindow* window)
    {
        return window->isCloseButtonPressed;
    }

    u32 platform_window_get_current_width(PlatformWindow* window)
    {
        return window->width;
    }

    u32 platform_window_get_current_height(PlatformWindow* window)
    {
        return window->height;
    }

    void platform_window_set_resize_callback(PlatformWindow* window, const Function<void()>& callback)
    {
        window->resizeCallback = callback;
    }

    LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        PlatformWindow* window = reinterpret_cast<PlatformWindow*>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
        if (window)
        {
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
            case WM_CLOSE:
                window->isCloseButtonPressed = true;
                return 0;
            case WM_SIZE:
                window->width = static_cast<u32>(LOWORD(lParam));
                window->height = static_cast<u32>(HIWORD(lParam));
                if (window->resizeCallback)
                {
                    window->resizeCallback();
                }
                return 0;
            // probably doesn't need to be processed
            // case WM_DESTROY:
            // case WM_QUIT:
            }
        }
        return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    bool process_mouse_input(PlatformWindow* window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        #define BIT(bit) (1 << bit)
        #define LOGMSG(msg, ...)
        switch (message)
        {
            case WM_LBUTTONDOWN:
            {
                window->input.mouse.buttons |= BIT(static_cast<u32>(MouseInput::LMB));
                LOGMSG(WM_LBUTTONDOWN)
                break;
            }
            case WM_LBUTTONUP:
            {
                window->input.mouse.buttons &= ~BIT(static_cast<u32>(MouseInput::LMB));
                LOGMSG(WM_LBUTTONUP)
                break;
            }
            case WM_MBUTTONDOWN:
            {
                window->input.mouse.buttons |= BIT(static_cast<u32>(MouseInput::MMB));
                LOGMSG(WM_MBUTTONDOWN)
                break;
            }
            case WM_MBUTTONUP:
            {
                window->input.mouse.buttons &= ~BIT(static_cast<u32>(MouseInput::MMB));
                LOGMSG(WM_MBUTTONUP)
                break;
            }
            case WM_RBUTTONDOWN:
            {
                window->input.mouse.buttons |= BIT(static_cast<u32>(MouseInput::RMB));
                LOGMSG(WM_RBUTTONDOWN)
                break;
            }
            case WM_RBUTTONUP:
            {
                window->input.mouse.buttons &= ~BIT(static_cast<u32>(MouseInput::RMB));
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
        #undef BIT
    }

    bool process_keyboard_input(PlatformWindow* window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        #define BIT(bit) (1ULL << bit)
        #define k(key) static_cast<u32>(KeyboardInput::key)
        static u64 VK_CODE_TO_KEYBOARD_FLAGS[] =
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
        switch (message)
        {
            case WM_KEYDOWN:
            {
                u64 flag = VK_CODE_TO_KEYBOARD_FLAGS[wParam];
                u64 id = flag / 64ULL;
                window->input.keyboard.buttons[id] |= BIT(flag - id * 64);
                break;
            }
            case WM_KEYUP:
            {
                u64 flag = VK_CODE_TO_KEYBOARD_FLAGS[wParam];
                u64 id = flag / 64ULL;
                window->input.keyboard.buttons[id] &= ~BIT(flag - id * 64);
                break;
            }
            default:
            {
                return false;
            }
        }
        return true;
        #undef k
        #undef BIT
    }
}
