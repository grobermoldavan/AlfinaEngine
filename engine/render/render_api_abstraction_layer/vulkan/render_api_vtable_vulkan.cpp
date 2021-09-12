
#include "render_api_vtable_vulkan.h"
#include "render_device_vulkan.h"
#include "render_program_vulkan.h"
#include "texture_vulkan.h"
#include "render_pass_vulkan.h"
#include "render_pipeline_vulkan.h"
#include "framebuffer_vulkan.h"
#include "render_pipeline_vulkan.h"
#include "command_buffer_vulkan.h"

namespace al
{
    void vulkan_render_api_vtable_fill(RenderApiVtable* table)
    {
        *table =
        {
            .device_create                          = vulkan_device_create,
            .device_destroy                         = vulkan_device_destroy,
            .device_wait                            = vulkan_device_wait,
            .get_swap_chain_textures_num            = vulkan_get_swap_chain_textures_num,
            .get_swap_chain_texture                 = vulkan_get_swap_chain_texture,
            .get_active_swap_chain_texture_index    = vulkan_get_active_swap_chain_texture_index,
            .begin_frame                            = vulkan_begin_frame,
            .end_frame                              = vulkan_end_frame,
            .program_create                         = vulkan_render_program_create,
            .program_destroy                        = vulkan_render_program_destroy,
            .texture_create                         = vulkan_texture_create,
            .texture_destroy                        = vulkan_texture_destroy,
            .render_pass_create                     = vulkan_render_pass_create,
            .render_pass_destroy                    = vulkan_render_pass_destroy,
            .render_pipeline_graphics_create        = vulkan_render_pipeline_graphics_create,
            .render_pipeline_destroy                = vulkan_render_pipeline_destroy,
            .framebuffer_create                     = vulkan_framebuffer_create,
            .framebuffer_destroy                    = vulkan_framebuffer_destroy,
            .command_buffer_request                 = vulkan_command_buffer_request,
            .command_buffer_submit                  = vulkan_command_buffer_submit,
            .command_bind_pipeline                  = vulkan_command_buffer_bind_pipeline,
            .command_draw                           = vulkan_command_buffer_draw,
        };
    }
}
