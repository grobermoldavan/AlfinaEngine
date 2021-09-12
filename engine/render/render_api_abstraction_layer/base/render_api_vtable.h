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
    struct RenderPipeline;
    struct GraphicsRenderPipelineCreateInfo;
    struct Framebuffer;
    struct FramebufferCreateInfo;
    struct CommandBuffer;
    struct CommandBufferRequestInfo;
    struct CommandBindPipelineInfo;
    struct CommandDrawInfo;

    struct RenderApiVtable
    {
        RenderDevice*           (*device_create)                        (RenderDeviceCreateInfo* createInfo);
        void                    (*device_destroy)                       (RenderDevice* device);
        void                    (*device_wait)                          (RenderDevice* device);
        uSize                   (*get_swap_chain_textures_num)          (RenderDevice* device);
        Texture*                (*get_swap_chain_texture)               (RenderDevice* device, uSize textureIndex);
        uSize                   (*get_active_swap_chain_texture_index)  (RenderDevice* device);
        void                    (*begin_frame)                          (RenderDevice* device);
        void                    (*end_frame)                            (RenderDevice* device);
        RenderProgram*          (*program_create)                       (RenderProgramCreateInfo* createInfo);
        void                    (*program_destroy)                      (RenderProgram* program);
        Texture*                (*texture_create)                       (TextureCreateInfo* createInfo);
        void                    (*texture_destroy)                      (Texture* texture);
        RenderPass*             (*render_pass_create)                   (RenderPassCreateInfo* createInfo);
        void                    (*render_pass_destroy)                  (RenderPass* pass);
        RenderPipeline*         (*render_pipeline_graphics_create)      (GraphicsRenderPipelineCreateInfo* createInfo);
        void                    (*render_pipeline_destroy)              (RenderPipeline* piepline);
        Framebuffer*            (*framebuffer_create)                   (FramebufferCreateInfo* createInfo);
        void                    (*framebuffer_destroy)                  (Framebuffer* framebuffer);
        CommandBuffer*          (*command_buffer_request)               (CommandBufferRequestInfo* requestInfo);
        void                    (*command_buffer_submit)                (CommandBuffer* buffer);
        void                    (*command_bind_pipeline)                (CommandBuffer* buffer, CommandBindPipelineInfo* commandInfo);
        void                    (*command_draw)                         (CommandBuffer* buffer, CommandDrawInfo* commandInfo);
    };
}

#endif
