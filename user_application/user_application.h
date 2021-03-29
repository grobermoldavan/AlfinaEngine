#ifndef USER_APPLICATION_H
#define USER_APPLICATION_H

#define AL_IMPLEMENTATION
#include "engine/engine.h"

template<typename Bindings> void user_create                (al::Application<Bindings>* application, al::CommandLineArgs args);
template<typename Bindings> void user_destroy               (al::Application<Bindings>* application);
template<typename Bindings> void user_update                (al::Application<Bindings>* application);
template<typename Bindings> void user_handle_window_resize  (al::Application<Bindings>* application);

struct UserBindings
{
    void(*create)               (al::Application<UserBindings>*, al::CommandLineArgs)   = user_create<UserBindings>;
    void(*destroy)              (al::Application<UserBindings>*)                        = user_destroy<UserBindings>;
    void(*update)               (al::Application<UserBindings>*)                        = user_update<UserBindings>;
    void(*handle_window_resize) (al::Application<UserBindings>*)                        = user_handle_window_resize<UserBindings>;
};

#endif
