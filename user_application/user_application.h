#ifndef USER_APPLICATION_H
#define USER_APPLICATION_H

#define AL_IMPLEMENTATION
#include "engine/engine.h"

// Application type forward declaration
struct UserApplication;

struct UserApplicationSubsystem
{
    al::uSize frameCounter;
};

void construct(UserApplicationSubsystem*, UserApplication*);
void destroy(UserApplicationSubsystem*, UserApplication*);
void update(UserApplicationSubsystem*, UserApplication*);
void resize(UserApplicationSubsystem*, UserApplication*);

struct UserRenderSubsystem
{
    al::RenderProgram* vs;
    al::RenderProgram* fs;
    al::RenderPass* renderPass;
    al::RenderPipeline* renderPipeline;
    al::Array<al::Texture*> swapChainTextures;
    al::Array<al::Framebuffer*> swapChainFramebuffers;
};

void construct(UserRenderSubsystem*, UserApplication*);
void destroy(UserRenderSubsystem*, UserApplication*);
void update(UserRenderSubsystem*, UserApplication*);
void resize(UserApplicationSubsystem*, UserApplication*);

// Declare bindings - this will hold pointers to user functions. Engine tries to match functions by name
struct UserBindings
{
    // declaring ApplicationType is required, so engine will be able to cast
    // base al::Application pointer to ApplicationType pointer when calling user functions
    using ApplicationType = UserApplication;

    al::ApplicationSubsystems
    <
        UserApplicationSubsystem,
        UserRenderSubsystem
    > subsystems;
};

// Actually declare user application structure
struct UserApplication : al::Application<UserBindings>
{

};

#endif
