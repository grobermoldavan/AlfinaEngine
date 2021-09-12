#ifndef AL_API_ABSTRACTION_LAYER_H
#define AL_API_ABSTRACTION_LAYER_H

#include "base/spirv_reflection/spirv_reflection.h"

#include "base/render_api_vtable.h"
#include "base/render_device.h"
#include "base/memory_buffer.h"
#include "base/render_program.h"
#include "base/render_pass.h"
#include "base/render_pipeline.h"
#include "base/texture.h"
#include "base/framebuffer.h"
#include "base/command_buffer.h"

#include "vulkan/vulkan_base.h"
#include "vulkan/vulkan_utils.h"
#include "vulkan/vulkan_memory_manager.h"
#include "vulkan/render_device_vulkan.h"
#include "vulkan/memory_buffer_vulkan.h"
#include "vulkan/render_program_vulkan.h"
#include "vulkan/render_pass_vulkan.h"
#include "vulkan/render_pipeline_vulkan.h"
#include "vulkan/texture_vulkan.h"
#include "vulkan/framebuffer_vulkan.h"
#include "vulkan/command_buffer_vulkan.h"
#include "vulkan/render_api_vtable_vulkan.h"

namespace al
{
    enum struct RenderApi
    {
        VULKAN,
    };

    void render_api_vtable_fill(RenderApiVtable* table, RenderApi api);
}

#endif
