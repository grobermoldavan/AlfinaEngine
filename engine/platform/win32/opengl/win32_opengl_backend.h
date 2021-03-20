#ifndef AL_WIN32_OPENGL_BACKEND_H
#define AL_WIN32_OPENGL_BACKEND_H

#define GLEW_STATIC
#include <GL/glew.h>
#include <GL/wglew.h>
#include <GL/GL.h>

#include "engine/platform/win32/win32_backend.h"
#include "engine/memory/pool_allocator.h"
#include "engine/debug/debug.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_MALLOC(size)       al::engine::allocate_using_allocation_info(&al::engine::gMemoryManager->pool, size);
#define STBI_REALLOC(ptr, size) al::engine::reallocate_using_allocation_info(&al::engine::gMemoryManager->pool, ptr, size);
#define STBI_FREE(ptr)          al::engine::deallocate_using_allocation_info(&al::engine::gMemoryManager->pool, ptr);
#define STBI_ASSERT(cond)       al_assert(cond)
#include "engine/3d_party_libs/stb/stb_image.h"

// @NOTE :  Current opengl rendering abstraction is based on public version of Hazel Engine. 
//          Source code for this engine is available here : https://github.com/TheCherno/Hazel

#endif
