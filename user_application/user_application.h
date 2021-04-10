#ifndef USER_APPLICATION_H
#define USER_APPLICATION_H

#define AL_IMPLEMENTATION
#include "engine/engine.h"

using UserApplication = al::Application<struct UserBindings>;

void user_create                (UserApplication* application, al::CommandLineArgs args);
void user_destroy               (UserApplication* application);
void user_update                (UserApplication* application);
void user_handle_window_resize  (UserApplication* application);

struct UserBindings
{
    void(*create)               (UserApplication*, al::CommandLineArgs)   = user_create;
    void(*destroy)              (UserApplication*)                        = user_destroy;
    void(*update)               (UserApplication*)                        = user_update;
    void(*handle_window_resize) (UserApplication*)                        = user_handle_window_resize;
};

#endif
