#ifndef AL_OS_WINDOW_WIN32_H
#define AL_OS_WINDOW_WIN32_H

#include "engine/platform/win32/win32_backend.h"
#include "engine/window/os_window.h"

namespace al::engine
{
    LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept;

    class OsWindowWin32 : public OsWindow
    {
    public:
        OsWindowWin32(OsWindowParams* params) noexcept;
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
}

#endif
