
#include "user_application.h"

#include <cstdio>

template<typename Bindings>
void user_create(al::Application<Bindings>* application, al::CommandLineArgs args)
{
    std::fprintf(stdout, "User create function\n");
}

template<typename Bindings>
void user_destroy(al::Application<Bindings>* application)
{
    std::fprintf(stdout, "User destroy function\n");
}

template<typename Bindings>
void user_update(al::Application<Bindings>* application)
{
    
}

template<typename Bindings>
void user_handle_window_resize(al::Application<Bindings>* application)
{
    using namespace al;
    u32 width = platform_window_get_current_width(&application->window);
    u32 height = platform_window_get_current_height(&application->window);
    std::fprintf(stdout, "User window resized function : %d %d\n", width, height);
}

int main(int argc, char* argv[])
{
    using namespace al;
    Application<UserBindings> userApplication = { };
    application_run(&userApplication, { argc, argv });
}
