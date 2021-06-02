#ifndef AL_SPIRV_REFLECTION_H
#define AL_SPIRV_REFLECTION_H

#include "engine/types.h"
#include "engine/utilities/utilities.h"

#include <spirv-headers/spirv.h>

#ifndef al_srs_printf
#   include <cstdio>
#   define al_srs_printf(fmt, ...) std::printf(fmt, __VA_ARGS__)
#endif

#ifndef al_srs_assert
#   include <cassert>
#   define al_srs_assert(condition) assert(condition)
#endif

#ifndef al_srs_array_size
#   define al_srs_array_size(array) sizeof(array) / sizeof(array[0])
#endif

#ifndef al_srs_indent
#   define al_srs_indent "    "
#endif

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
        struct TypeInfo
        {
            enum struct ScalarKind { INTEGER, FLOAT, };
            enum Kind { SCALAR, VECTOR, MATRIX, STRUCTURE, ARRAY };
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
                struct Array
                {
                    TypeInfo* entryType;
                    uSize size;
                } array;
            } info;
            Kind kind;
            TypeInfo* next;
        };
        struct ShaderIO
        {
            enum Flags { IS_BUILT_IN, };
            const char* name;
            TypeInfo* typeInfo;
            u32 location;
            u32 flags;
        };
        struct Image
        {
            enum Flags { IS_SAMPLED, IS_SUBPASS_DEPENDENCY, };
            u32 flags;
        };
        struct Uniform
        {
            enum Type { IMAGE, UNIFORM_BUFFER, STORAGE_BUFFER };
            const char* name;
            Type type;
            union
            {
                Image image;
                TypeInfo* buffer;
            };
            u32 set;
            u32 binding;
        };
        struct PushConstant
        {
            const char* name;
            TypeInfo* typeInfo;
        };

        FixedSizeBuffer<DYNAMIC_BUFFER_BUFFER_SIZE> buffer;

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

    void construct_spirv_reflection(SpirvReflection* reflection, AllocatorBindings bindings, SpirvWord* bytecode, uSize wordCount);
    void print_type_info(SpirvReflection::TypeInfo* typeInfo, SpirvReflection* reflection, uSize indentationLevel = 0, const char* structMemberName = nullptr);
    uSize get_type_info_size(SpirvReflection::TypeInfo* typeInfo);

    const char* shader_type_to_str(SpirvReflection::ShaderType type);
}

#endif
