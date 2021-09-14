#ifndef USER_APPLICATION_H
#define USER_APPLICATION_H

#define AL_IMPLEMENTATION
#include "engine/engine.h"

// Application type forward declaration
struct UserApplication;

// User functions forward declaration
void user_create                (UserApplication* application, al::CommandLineArgs args);
void user_destroy               (UserApplication* application);
void user_update                (UserApplication* application);
void user_handle_window_resize  (UserApplication* application);
void user_renderer_construct    (UserApplication* application);
void user_renderer_destroy      (UserApplication* application);
void user_renderer_render       (UserApplication* application);

// Declare bindings - this will hold pointers to user functions. Engine tries to match functions by name
struct UserBindings
{
    // declaring ApplicationType is required, so engine will be able to cast
    // base al::Application pointer to ApplicationType pointer when calling user functions
    using ApplicationType = UserApplication;

    void(*application_create)   (UserApplication*, al::CommandLineArgs) = user_create;
    void(*application_destroy)  (UserApplication*)                      = user_destroy;
    void(*application_update)   (UserApplication*)                      = user_update;
    void(*handle_window_resize) (UserApplication*)                      = user_handle_window_resize;
    void(*renderer_construct)   (UserApplication*)                      = user_renderer_construct;
    void(*renderer_destroy)     (UserApplication*)                      = user_renderer_destroy;
    void(*renderer_render)      (UserApplication*)                      = user_renderer_render;
};

// Actually declare user application structure
struct UserApplication : al::Application<UserBindings>
{
    al::RenderProgram* vs;
    al::RenderProgram* fs;
    al::RenderPass* renderPass;
    al::RenderPipeline* renderPipeline;
    al::Array<al::Texture*> swapChainTextures;
    al::Array<al::Framebuffer*> swapChainFramebuffers;
};

#endif
