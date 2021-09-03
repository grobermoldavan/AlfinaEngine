#ifndef AL_SPIRV_REFLECTION_H
#define AL_SPIRV_REFLECTION_H

#include "engine/types.h"
#include "engine/memory/memory.h"

#include <spirv-headers/spirv.h>

#ifndef al_srs_printf
#   include <cstdio>
#   define al_srs_printf(fmt, ...) std::printf(fmt, __VA_ARGS__)
#endif

#ifndef al_srs_assert
#   include <cassert>
#   define al_srs_assert(condition) assert(condition)
#endif

#ifndef al_srs_memset
#   include <cstring>
#   define al_srs_memset(ptr, val, size) std::memset(ptr, val, size)
#endif

#ifndef al_srs_array_size
#   define al_srs_array_size(array) sizeof(array) / sizeof(array[0])
#endif

#ifndef al_srs_indent
#   define al_srs_indent "    "
#endif

namespace al
{
    using SpirvWord = u32;

    struct SpirvReflection
    {
        static constexpr uSize MEMORY_BUFFER_SIZE = 1024;
        struct MemoryBuffer
        {
            uSize size;
            uSize capacity;
            MemoryBuffer* next;
            u8 memory[MEMORY_BUFFER_SIZE];
        };
        enum ShaderType : u64
        {
            Vertex,
            Fragment,
        };
        struct TypeInfo
        {
            enum Kind : u64 { Scalar, Vector, Matrix, Structure, Sampler, Image, Array };
            enum struct ScalarKind : u16 { Integer, Float, };
            enum struct ArrayKind : u64 { RuntimeArray, Array, };
            enum struct ImageKind : u8 { SampledImage, Image, };
            enum struct ImageDim : u8 { _1D, _2D, _3D, Cube, Rect, Buffer, SubpassData };
            enum struct ImageDepthParameters : u8 { DepthImage, NotDepthImage, Unknown };
            enum struct ImageSampleParameters : u8 { UsedWithSampler, NotUsedWithSampler, Unknown };
            union
            {
                struct
                {
                    ScalarKind kind;
                    u8 bitWidth;
                    bool isSigned;
                } scalar;
                struct
                {
                    TypeInfo* componentType;
                    u8 componentsCount;
                } vector;
                struct
                {
                    TypeInfo* columnType;
                    u8 columnsCount;
                } matrix;
                struct Structure
                {
                    const char* typeName;
                    TypeInfo** members;
                    const char** memberNames;
                    uSize membersCount;
                } structure;
                struct Sampler
                {
                    // Nothing (?)
                } sampler;
                struct Image
                {
                    TypeInfo* sampledType;
                    ImageKind kind;
                    ImageDim dim;
                    ImageDepthParameters depthParameters;
                    bool isArrayed;
                    bool isMultisampled;
                    ImageSampleParameters sampleParameters;
                } image;
                struct Array
                {
                    TypeInfo* entryType;
                    uSize size;
                    ArrayKind kind;
                } array;
            } info;
            Kind kind;
            TypeInfo* next;
        };
        struct ShaderIO
        {
            enum Flags : u32 { IsBuiltIn = u32(1) << 0, };
            const char* name;
            TypeInfo* typeInfo;
            u32 location;
            u32 flags;
        };
        struct Uniform
        {
            enum Type : u32
            {
                Sampler,
                SampledImage,
                StorageImage,
                CombinedImageSampler,
                UniformTexelBuffer,
                StorageTexelBuffer,
                UniformBuffer,
                StorageBuffer,
                InputAttachment,
                AccelerationStructure,
            };
            const char* name;
            TypeInfo* typeInfo;
            Type type;
            u32 set;
            u32 binding;
            u32 inputAttachmentIndex;
        };
        struct PushConstant
        {
            const char* name;
            TypeInfo* typeInfo;
        };

        AllocatorBindings allocator;
        MemoryBuffer* memory;

        TypeInfo* typeInfos; // stored as linked list

        ShaderType shaderType;
        const char* entryPointName;

        ShaderIO* shaderInputs;
        uSize shaderInputCount;

        ShaderIO* shaderOutputs;
        uSize shaderOutputCount;

        Uniform* uniforms;
        uSize uniformCount;

        PushConstant* pushConstant;
    };

    struct SpirvReflectionCreateInfo
    {
        AllocatorBindings* persistentAllocator;
        AllocatorBindings* nonPersistentAllocator;
        SpirvWord* bytecode;
        uSize wordCount;
    };

    void spirv_reflection_construct(SpirvReflection* reflection, SpirvReflectionCreateInfo* createInfo);
    void spirv_reflection_destroy(SpirvReflection* reflection);
    void spirv_reflection_print_type_info(SpirvReflection::TypeInfo* typeInfo, SpirvReflection* reflection, uSize indentationLevel = 0, const char* structMemberName = nullptr);
    uSize spirv_reflection_get_type_info_size(SpirvReflection::TypeInfo* typeInfo);

    const char* spirv_reflection_shader_type_to_str(SpirvReflection::ShaderType type);
}

#endif
