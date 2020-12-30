
#include "win32_opengl_texture_2d.h"

#include "engine/debug/debug.h"
#include "engine/memory/memory_manager.h"

namespace al::engine
{
    template<> [[nodiscard]] Texture2d* create_texture_2d<RendererType::OPEN_GL>(const char* path) noexcept
    {
        Texture2d* tex = MemoryManager::get()->get_pool()->allocate_as<Win32OpenglTexure2d>();
        ::new(tex) Win32OpenglTexure2d{ path };
        return tex;
    }

    template<> void destroy_texture_2d<RendererType::OPEN_GL>(Texture2d* tex) noexcept
    {
        tex->~Texture2d();
        MemoryManager::get()->get_pool()->deallocate(reinterpret_cast<std::byte*>(tex), sizeof(Win32OpenglTexure2d));
    }

    Win32OpenglTexure2d::Win32OpenglTexure2d(const char* path) noexcept
    {
        al_profile_function();

        int32_t stbi_width;
        int32_t stbi_height;
        int32_t stbi_channels;

        // @NOTE :  stbi reads image in different order compared to the one, that opengl expects,
        //          so image needs to be flipped
        ::stbi_set_flip_vertically_on_load(1);
        stbi_uc* data = ::stbi_load(path, &stbi_width, &stbi_height, &stbi_channels, 0);

        // Unable to load the image
        al_assert(data);

        width = static_cast<uint32_t>(stbi_width);
        height = static_cast<uint32_t>(stbi_height);

        if (stbi_channels == 4)
        {
            internalFormat = GL_RGBA8;
            dataFormat = GL_RGBA;
        }
        else if (stbi_channels == 3)
        {
            internalFormat = GL_RGB8;
            dataFormat = GL_RGB;
        }
        else
        {
            // Unsupported number of channels
            al_assert(false);
        }

        ::glCreateTextures(GL_TEXTURE_2D, 1, &rendererId);
        ::glTextureStorage2D(rendererId, 1, internalFormat, width, height);

        ::glTextureParameteri(rendererId, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ::glTextureParameteri(rendererId, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        ::glTextureParameteri(rendererId, GL_TEXTURE_WRAP_S, GL_REPEAT);
        ::glTextureParameteri(rendererId, GL_TEXTURE_WRAP_T, GL_REPEAT);

        ::glTextureSubImage2D(rendererId, 0, 0, 0, width, height, dataFormat, GL_UNSIGNED_BYTE, data);

        ::stbi_image_free(data);
    }

    Win32OpenglTexure2d::~Win32OpenglTexure2d() noexcept
    {
        al_profile_function();

        ::glDeleteTextures(1, &rendererId);
    }

    void Win32OpenglTexure2d::bind(uint32_t slot) const noexcept
    {
        ::glBindTextureUnit(slot, rendererId);
    }

    void Win32OpenglTexure2d::unbind() const noexcept
    {
        // nothing
    }
}
