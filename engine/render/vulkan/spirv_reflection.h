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
        static constexpr uSize DYNAMIC_BUFFER_BUFFER_SIZE = 4096;
        enum ShaderType
        {
            VERTEX,
            FRAGMENT,
        };
        struct ShaderInput
        {
            const char* name;
            u32 location;
            u32 sizeBytes;
        };
        struct ShaderStruct
        {
            struct Member
            {
                const char* name;
                uSize sizeBytes;
            };
            const char* name;
            Member* members;
            uSize membersNum;
        };
        struct Image
        {
            enum Flags
            {
                IS_SAMPLED,
                IS_SUBPASS_DEPENDENCY,
            };
            u32 flags;
        };
        struct Uniform
        {
            enum Type
            {
                IMAGE,
                BUFFER,
            };
            const char* name;
            Type type;
            union
            {
                ShaderStruct* buffer;
                Image* image;
            };
            u32 set;
            u32 binding;
            // u32 inputAttachmentIndex; ????
        };
        u8 dynamicMemoryBuffer[DYNAMIC_BUFFER_BUFFER_SIZE];
        uSize dynamicMemoryBufferCounter;

        ShaderType shaderType;
        const char* entryPointName;

        ShaderInput* shaderInputs;
        uSize shaderInputCount;

        Uniform* uniforms;
        uSize uniformCount;

        ShaderStruct* pushConstant;
    };

    struct SpirvId
    {
        enum Flags
        {
            IS_INPUT                = 0,
            IS_OUTPUT               = 1,
            IS_PUSH_CONSTANT        = 2,
            IS_UNIFORM              = 3,
            HAS_LOCATION_DECORATION = 4,
            HAS_BINDING_DECORATION  = 5,
            HAS_SET_DECORATION      = 6,
        };
        SpirvWord* declarationLocation;
        char* name;
        u32 flags;
        u32 location;   // decoration
        u32 binding;    // decoration
        u32 set;        // decoration
    };

    struct SpirvStruct
    {
        struct Member
        {
            SpirvId* id;
            char* name;
        };
        SpirvId* id;
        uSize membersNum;
        Member* members;
    };

    SpirvReflection::ShaderType execution_mode_to_shader_type(SpvExecutionModel model);
    const char* op_code_to_str(u16 opCode);

    u8*     allocate_data_in_buffer (uSize dataSize, u8* buffer, uSize* bufferPtr, uSize bufferSize);
    u8*     allocate_data_in_buffer (uSize dataSize, SpirvReflection* reflection);
    u8*     save_data_to_buffer     (void* data, uSize dataSize, u8* buffer, uSize* bufferPtr, uSize bufferSize);
    char*   save_string_to_buffer   (char* str, u8* buffer, uSize* bufferPtr, uSize bufferSize);
    char*   save_string_to_buffer   (char* str, SpirvReflection* reflection);

    uSize get_id_size(SpirvStruct* structs, uSize numStructs, SpirvId* ids, SpirvId* id);
    void save_runtime_struct_to_result_struct(SpirvStruct* structs, uSize structsNum, SpirvId* ids, SpirvStruct* runtimeStruct, SpirvReflection::ShaderStruct* resultStruct, SpirvReflection* reflection);
    SpirvStruct* find_struct_by_id(SpirvStruct* structs, uSize numStructs, SpirvId* id);

    void save_runtime_struct_to_result_struct(SpirvStruct* runtimeStruct, SpirvReflection::ShaderStruct* resultStruct, SpirvReflection* reflection);

    void process_shader_input_variable(SpirvStruct* structs, uSize structsNum, SpirvId* ids, SpirvId* inputVariable, SpirvReflection* reflection);
    void process_shader_push_constant(SpirvStruct* structs, uSize structsNum, SpirvId* ids, SpirvId* pushConstant, SpirvReflection* reflection);
    void process_shader_uniform_buffer(SpirvStruct* structs, uSize structsNum, SpirvId* ids, SpirvId* uniform, SpirvReflection* reflection);
    u32 get_storage_class_flags(SpirvWord word);

    void construct_spirv_reflecttion(SpirvReflection* reflection, AllocatorBindings bindings, SpirvWord* bytecode, uSize wordCount);

    const char* shader_type_to_str(SpirvReflection::ShaderType type);
}

#endif
