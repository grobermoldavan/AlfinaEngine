
#ifdef _WIN32
#   include "engine/platform/win32/platform_input_win32.cpp"
#   include "engine/platform/win32/platform_window_win32.cpp"
#else
#   error Unsupported platform
#endif
