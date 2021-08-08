
#include "render_program_vulkan.h"
#include "render_device_vulkan.h"
#include "../base/spirv_reflection/spirv_reflection.h"

namespace al
{
    RenderProgram* vulkan_render_program_create(RenderProgramCreateInfo* createInfo)
    {
        RenderDeviceVulkan* device = (RenderDeviceVulkan*)createInfo->device;
        RenderProgramVulkan* program = allocate<RenderProgramVulkan>(&device->memoryManager.cpu_persistentAllocator);
        Tuple<u32*, uSize> u32bytecode { createInfo->bytecode, createInfo->codeSizeBytes };
        program->device = device;
        program->handle = utils::create_shader_module(device->gpu.logicalHandle, u32bytecode, &device->memoryManager.cpu_allocationCallbacks);

        SpirvReflection reflection;
        u8* reflectionMemory = allocate<u8>(&device->memoryManager.cpu_persistentAllocator, RenderProgramVulkan::REFLECTION_BUFFER_SIZE_BYTES);
        SpirvReflectionCreateInfo reflectionCreateInfo
        {
            .persistentAllocator = &device->memoryManager.cpu_persistentAllocator,
            .nonPersistentAllocator = &device->memoryManager.cpu_frameAllocator,
            .bytecode = createInfo->bytecode,
            .wordCount = createInfo->codeSizeBytes / sizeof(SpirvWord),
        };
        spirv_reflection_construct(&reflection, &reflectionCreateInfo);

        return program;
    }

    void vulkan_render_program_destroy(RenderProgram* _program)
    {
        RenderProgramVulkan* program = (RenderProgramVulkan*)_program;
        utils::destroy_shader_module(program->device->gpu.logicalHandle, program->handle, &program->device->memoryManager.cpu_allocationCallbacks);
        spirv_reflection_destroy(&program->reflection);
        deallocate<RenderProgramVulkan>(&program->device->memoryManager.cpu_persistentAllocator, program);
    }
}
