#ifndef AL_RENDER_API_VTABLE_H
#define AL_RENDER_API_VTABLE_H

#include "engine/types.h"
#include "enums.h"

namespace al
{
    struct RenderDevice;
    struct RenderDeviceCreateInfo;
    struct RenderProgram;
    struct RenderProgramCreateInfo;
    struct Texture;
    struct TextureCreateInfo;
    struct RenderPass;
    struct RenderPassCreateInfo;
    struct Framebuffer;
    struct FramebufferCreateInfo;
    // struct RenderStage;
    // struct GraphicsRenderStageCreateInfo;
    struct RenderCommandBuffer;
    struct RenderCommandBufferCreateInfo;

    struct RenderApiVtable
    {
        RenderDevice*           (*device_create)                    (RenderDeviceCreateInfo* createInfo);
        void                    (*device_destroy)                   (RenderDevice* device);
        uSize                   (*get_swap_chain_textures_num)      (RenderDevice* device);
        Texture*                (*get_swap_chain_texture)           (RenderDevice* device, uSize textureIndex);
        RenderProgram*          (*program_create)                   (RenderProgramCreateInfo* createInfo);
        void                    (*program_destroy)                  (RenderProgram* program);
        Texture*                (*texture_create)                   (TextureCreateInfo* createInfo);
        void                    (*texture_destroy)                  (Texture* texture);
        RenderPass*             (*render_pass_create)               (RenderPassCreateInfo* createInfo);
        void                    (*render_pass_destroy)              (RenderPass* pass);
        Framebuffer*            (*framebuffer_create)               (FramebufferCreateInfo* createInfo);
        void                    (*framebuffer_destroy)              (Framebuffer* framebuffer);
        // RenderStage*            (*stage_graphics_create)            (GraphicsRenderStageCreateInfo* createInfo);
        // void                    (*stage_destroy)                    (RenderStage* stage);
        RenderCommandBuffer*    (*command_buffer_create)            (RenderCommandBufferCreateInfo* createInfo);
        void                    (*command_buffer_destroy)           (RenderCommandBuffer* buffer);
        void                    (*command_buffer_begin)             (RenderCommandBuffer* buffer);
        void                    (*command_buffer_submit)            (RenderCommandBuffer* buffer);
        // void                    (*cmd_bind_stage)                   (RenderCommandBuffer* buffer, RenderStage* stage);
        void                    (*cmd_draw)                         (RenderCommandBuffer* buffer, uSize numVertices);
    };
}

#endif
