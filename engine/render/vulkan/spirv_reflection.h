#ifndef AL_SPIRV_REFLECTION_H
#define AL_SPIRV_REFLECTION_H

#include "engine/types.h"
#include "engine/utilities/utilities.h"
#include "vulkan_base.h"

namespace al::vulkan
{
    using SpirvWord = u32;

    struct SpirvReflection
    {
        static constexpr uSize NAME_BUFFER_SIZE = 256;
        static constexpr uSize MAX_VERTEX_INPUTS = 8;
        enum ShaderType
        {
            VERTEX,
            FRAGMENT,
        };
        struct ShaderInput
        {
            char* name;
            u32 location;
            u32 sizeBytes;
        };
        struct PushConstant
        {
            char* name;
            u32 sizeBytes;
        };
        char nameBuffer[NAME_BUFFER_SIZE];
        uSize nameBufferCounter;
        ShaderType shaderType;
        char* entryPointName;
        ShaderInput shaderInputs[MAX_VERTEX_INPUTS];
        uSize shaderInputCount;
        PushConstant pushConstant;
    };

    struct SpirvId
    {
        enum Flags
        {
            IS_INPUT            = 0,
            IS_OUTPUT           = 1,
            IS_PUSH_CONSTANT    = 2,
            IS_UNIFORM          = 3,
        };
        SpirvWord* declarationLocation;
        char* name;
        u16 location; // corresponds to "OpDecorate %var Location *location*"
        u32 flags;
    };

    SpirvReflection::ShaderType execution_mode_to_shader_type(SpvExecutionModel model);
    const char* op_code_to_str(u16 opCode);
    char* save_string_to_buffer(const char* str, char* buffer, uSize* bufferPtr, uSize bufferSize);

    void process_shader_input_variable(ArrayView<SpirvId> ids, SpirvId* inputVariable, SpirvReflection* reflection);

    void construct_spirv_reflect(AllocatorBindings bindings, SpirvWord* bytecode, uSize wordCount, SpirvReflection* result);
    const char* shader_type_to_str(SpirvReflection::ShaderType type);
}

#endif
