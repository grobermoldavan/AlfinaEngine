
#include "render_api_abstraction_layer.h"

namespace al
{
    void render_api_vtable_fill(RenderApiVtable* table, RenderApi api)
    {
        switch (api)
        {
            case RenderApi::VULKAN: vulkan_render_api_vtable_fill(table);
        }
    }
}

#include "base/spirv_reflection/spirv_reflection.cpp"

#include "vulkan/vulkan_utils.cpp"
#include "vulkan/vulkan_memory_manager.cpp"
#include "vulkan/render_device_vulkan.cpp"
#include "vulkan/memory_buffer_vulkan.cpp"
#include "vulkan/render_program_vulkan.cpp"
#include "vulkan/render_pass_vulkan.cpp"
#include "vulkan/render_pipeline_vulkan.cpp"
#include "vulkan/texture_vulkan.cpp"
#include "vulkan/framebuffer_vulkan.cpp"
#include "vulkan/command_buffer_vulkan.cpp"
#include "vulkan/render_api_vtable_vulkan.cpp"
