
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
        SpirvReflectionCreateInfo reflectionCreateInfo
        {
            .persistentAllocator = &device->memoryManager.cpu_persistentAllocator,
            .nonPersistentAllocator = &device->memoryManager.cpu_frameAllocator,
            .bytecode = createInfo->bytecode,
            .wordCount = createInfo->codeSizeBytes / sizeof(SpirvWord),
        };
        spirv_reflection_construct(&program->reflection, &reflectionCreateInfo);
        // // For debug
        // PointerWithSize<SpirvReflection::Uniform> uniforms { program->reflection.uniforms, program->reflection.uniformCount };
        // for (al_iterator(it, uniforms))
        // {
        //     auto* uniform = get(it);
        //     int a = 0;
        // }
        // PointerWithSize<SpirvReflection::ShaderIO> inputs { program->reflection.shaderInputs, program->reflection.shaderInputCount };
        // for (al_iterator(it, inputs))
        // {
        //     auto* input = get(it);
        //     int a = 0;
        // }
        // PointerWithSize<SpirvReflection::ShaderIO> outputs { program->reflection.shaderOutputs, program->reflection.shaderOutputCount };
        // for (al_iterator(it, outputs))
        // {
        //     auto* output = get(it);
        //     int a = 0;
        // }
        return program;
    }

    void vulkan_render_program_destroy(RenderProgram* _program)
    {
        RenderProgramVulkan* program = (RenderProgramVulkan*)_program;
        utils::destroy_shader_module(program->device->gpu.logicalHandle, program->handle, &program->device->memoryManager.cpu_allocationCallbacks);
        spirv_reflection_destroy(&program->reflection);
        deallocate<RenderProgramVulkan>(&program->device->memoryManager.cpu_persistentAllocator, program);
    }

    VulkanCombinedProgramInfo vulkan_combined_program_info_create(PointerWithSize<RenderProgramVulkan*> programs, AllocatorBindings* persistentAllocator)
    {
        using BitMask = u32;
        constexpr uSize MAX_SETS = 32;
        BitMask usedBindings[MAX_SETS] = {};
        //
        // Setup bindings bitmasks
        //
        for (al_iterator(programIt, programs))
        {
            RenderProgramVulkan* program = *get(programIt);
            PointerWithSize<SpirvReflection::Uniform> uniforms{ program->reflection.uniforms, program->reflection.uniformCount };
            for (al_iterator(uniformIt, uniforms))
            {
                SpirvReflection::Uniform* uniform = get(uniformIt);
                usedBindings[uniform->set] |= BitMask(1) << uniform->binding;
            }
        }
        //
        // Check for setup correctness. Currently empty descriptor sets in between of not empty ones are not supported
        //
        bool isEmptyMaskFound = false;
        for (al_iterator(bindingsIt, usedBindings))
        {
            const BitMask bindingMask = *get(bindingsIt);
            al_assert_msg(!isEmptyMaskFound || !bindingMask, "@TODO : support empty descriptor sets");
            isEmptyMaskFound = isEmptyMaskFound || !bindingMask;
        }
        //
        // Allocate memory for required data
        //
        Array<VkDescriptorSetLayoutCreateInfo> layoutCreateInfos{};
        Array<VkDescriptorSetLayoutBinding> layoutBindings{};
        for (al_iterator(bindingsIt, usedBindings))
        {
            const BitMask bindingMask = *get(bindingsIt);
            if (!bindingMask) continue;
            layoutCreateInfos.size += 1;
            for (uSize maskIt = 0; maskIt < sizeof(BitMask) * 8; maskIt++)
            {
                if (bindingMask & (BitMask(1) << maskIt)) layoutBindings.size += 1;
            }
        }
        array_construct(&layoutCreateInfos, persistentAllocator, layoutCreateInfos.size);
        array_construct(&layoutBindings, persistentAllocator, layoutBindings.size);
        for (al_iterator(it, layoutBindings)) get(it)->binding = ~u32(0); // binding value is not filled if it is equal to ~u32(0)
        //
        // Set layout create infos
        //
        uSize layoutCreateInfoIt = 0;
        uSize layoutBindingsIt = 0;
        for (al_iterator(bindingsIt, usedBindings))
        {
            const BitMask bindingMask = *get(bindingsIt);
            if (!bindingMask) continue;
            u32 numberOfBindingsInSet = 0;
            for (uSize maskIt = 0; maskIt < sizeof(BitMask) * 8; maskIt++)
            {
                if (bindingMask & (BitMask(1) << maskIt)) numberOfBindingsInSet += 1;
            }
            layoutCreateInfos[layoutCreateInfoIt++] =
            {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .bindingCount = numberOfBindingsInSet,
                .pBindings = &layoutBindings[layoutBindingsIt],
            };
            layoutBindingsIt += numberOfBindingsInSet;
        }
        //
        // Fill layout binding data
        //
        for (al_iterator(programIt, programs))
        {
            RenderProgramVulkan* program = *get(programIt);
            const VkShaderStageFlags programStage = [](const SpirvReflection::ShaderType shaderType) -> VkShaderStageFlags
            {
                switch (shaderType)
                {
                    case SpirvReflection::Vertex: return VK_SHADER_STAGE_VERTEX_BIT;
                    case SpirvReflection::Fragment: return VK_SHADER_STAGE_FRAGMENT_BIT;
                    default: al_assert_fail("Unsupported SpirvReflection::ShaderType");
                }
                return VkShaderStageFlags(0);
            }(program->reflection.shaderType);
            PointerWithSize<SpirvReflection::Uniform> uniforms{ program->reflection.uniforms, program->reflection.uniformCount };
            for (al_iterator(uniformIt, uniforms))
            {
                SpirvReflection::Uniform* uniform = get(uniformIt);
                const VkDescriptorType uniformDescriptorType = [](const SpirvReflection::Uniform::Type uniformType) -> VkDescriptorType
                {
                    switch (uniformType)
                    {
                        case SpirvReflection::Uniform::Sampler: return VK_DESCRIPTOR_TYPE_SAMPLER;
                        case SpirvReflection::Uniform::SampledImage: return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                        case SpirvReflection::Uniform::StorageImage: return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                        case SpirvReflection::Uniform::CombinedImageSampler: return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                        case SpirvReflection::Uniform::UniformTexelBuffer: return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
                        case SpirvReflection::Uniform::StorageTexelBuffer: return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
                        case SpirvReflection::Uniform::UniformBuffer: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                        case SpirvReflection::Uniform::StorageBuffer: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                        case SpirvReflection::Uniform::InputAttachment: return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
                        case SpirvReflection::Uniform::AccelerationStructure: return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR; // ?
                        default: al_assert_fail("Unsupported SpirvReflection::Uniform::Type");
                    }
                    return VkDescriptorType(0);
                }(uniform->type);
                uSize bindingArrayInitialOffset = layoutCreateInfos[uniform->set].pBindings - layoutBindings.memory;
                VkDescriptorSetLayoutBinding* binding = nullptr;
                for (uSize it = 0; it < layoutCreateInfos[uniform->set].bindingCount; it++)
                {
                    VkDescriptorSetLayoutBinding* candidate = &layoutBindings[bindingArrayInitialOffset + it];
                    if (candidate->binding == ~u32(0) || candidate->binding == uniform->binding)
                    {
                        binding = candidate;
                        break;
                    }
                }
                const u32 descriptorCount =
                    (uniform->typeInfo->kind == SpirvReflection::TypeInfo::Array) &&
                    (uniform->typeInfo->info.array.kind == SpirvReflection::TypeInfo::ArrayKind::Array)
                    ? u32(uniform->typeInfo->info.array.size)
                    : 1;
                if (binding->binding != ~u32(0)) 
                {
                    al_assert_msg
                    (
                        binding->descriptorType == uniformDescriptorType &&
                        binding->descriptorCount == descriptorCount,
                        "Same binding at the same location must have the same descriptor type in each shader"
                    );
                    binding->stageFlags |= programStage;
                }
                else
                {
                    
                    binding->binding = uniform->binding;
                    binding->descriptorType = uniformDescriptorType;
                    binding->descriptorCount = descriptorCount;
                    binding->stageFlags = programStage;
                }
            }
        }
        return { layoutCreateInfos, layoutBindings };
    }

    void vulkan_combined_program_info_destroy(VulkanCombinedProgramInfo* combinedInfo)
    {
        array_destruct(&combinedInfo->descriptorSetLayoutCreateInfos);
        array_destruct(&combinedInfo->descriptorSetLayoutBindings);
    }
}
