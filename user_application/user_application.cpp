
#include "user_application.h"

#include <cstdio>

void user_create(UserApplication* application, al::CommandLineArgs args)
{
    std::fprintf(stdout, "User create function\n");
}

void user_destroy(UserApplication* application)
{
    std::fprintf(stdout, "User destroy function\n");
}

void user_update(UserApplication* application)
{
    
}

void user_handle_window_resize(UserApplication* application)
{
    using namespace al;
    u32 width = platform_window_get_current_width(&application->window);
    u32 height = platform_window_get_current_height(&application->window);
    std::fprintf(stdout, "User window resized function : %d %d\n", width, height);
}

int main(int argc, char* argv[])
{
    using namespace al;
    UserApplication userApplication = { };
    application_run(&userApplication, { argc, argv });
}
