
#include "render_api_vtable_vulkan.h"
#include "render_device_vulkan.h"
#include "render_program_vulkan.h"
#include "texture_vulkan.h"
#include "render_pass_vulkan.h"
#include "render_pipeline_vulkan.h"
#include "framebuffer_vulkan.h"
#include "render_pipeline_vulkan.h"

namespace al
{
    void vulkan_render_api_vtable_fill(RenderApiVtable* table)
    {
        *table =
        {
            .device_create                      = vulkan_device_create,
            .device_destroy                     = vulkan_device_destroy,
            .get_swap_chain_textures_num        = vulkan_get_swap_chain_textures_num,
            .get_swap_chain_texture             = vulkan_get_swap_chain_texture,
            .program_create                     = vulkan_render_program_create,
            .program_destroy                    = vulkan_render_program_destroy,
            .texture_create                     = vulkan_texture_create,
            .texture_destroy                    = vulkan_texture_destroy,
            .render_pass_create                 = vulkan_render_pass_create,
            .render_pass_destroy                = vulkan_render_pass_destroy,
            .render_pipeline_graphics_create    = vulkan_render_pipeline_graphics_create,
            .render_pipeline_destroy            = vulkan_render_pipeline_destroy,
            .framebuffer_create                 = vulkan_framebuffer_create,
            .framebuffer_destroy                = vulkan_framebuffer_destroy,
            // .command_buffer_create              = nullptr,
            // .command_buffer_destroy             = nullptr,
            // .command_buffer_begin               = nullptr,
            // .command_buffer_submit              = nullptr,
            // .cmd_bind_stage                     = nullptr,
            // .cmd_draw                           = nullptr,
        };
    }
}
