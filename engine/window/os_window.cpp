
#include "os_window.h"

namespace al::engine
{
    const OsWindowParams* OsWindow::get_params() const noexcept
    {
        return &params;
    }

    OsWindow::OsWindow(const OsWindowParams& params) noexcept
        : params{ params }
    { }
}
