
#include <cstring>

#include "spirv_reflection.h"

#define OPCODE(word) u16(word)
#define WORDCOUNT(word) u16(instruction[0] >> 16)

namespace al::vulkan
{
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
            IS_BUILT_IN             = 7,
            IS_BLOCK                = 8,
            IS_BUFFER_BLOCK         = 9,
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
        uSize membersCount;
        Member* members;
    };

    static SpirvReflection::ShaderType execution_mode_to_shader_type(SpvExecutionModel model)
    {
        switch (model)
        {
            case SpvExecutionModelVertex:   return SpirvReflection::VERTEX;
            case SpvExecutionModelFragment: return SpirvReflection::FRAGMENT;
        }
        al_srs_assert(!"Unsupported SpvExecutionModel value");
        return SpirvReflection::ShaderType(0);
    }

    static char* save_string_to_buffer(const char* str, SpirvReflection* reflection)
    {
        uSize length = std::strlen(str);
        char* result = fixed_size_buffer_allocate<char>(&reflection->buffer, length + 1);
        std::memcpy(result, str, length);
        result[length] = 0;
        return result;
    }

    static SpirvStruct* find_struct_by_id(SpirvStruct* structs, uSize structsCount, SpirvId* id)
    {
        for (uSize it = 0; it < structsCount; it++)
        {
            if (structs[it].id == id)
            {
                return &structs[it];
            }
        }
        return nullptr;
    }

    static SpirvReflection::TypeInfo::Kind op_type_to_value_kind(SpirvWord word)
    {
        switch (OPCODE(word))
        {
            case SpvOpTypeFloat:        return SpirvReflection::TypeInfo::SCALAR;
            case SpvOpTypeInt:          return SpirvReflection::TypeInfo::SCALAR;
            case SpvOpTypeVector:       return SpirvReflection::TypeInfo::VECTOR;
            case SpvOpTypeMatrix:       return SpirvReflection::TypeInfo::MATRIX;
            case SpvOpTypeStruct:       return SpirvReflection::TypeInfo::STRUCTURE;
            case SpvOpTypeRuntimeArray: return SpirvReflection::TypeInfo::ARRAY;
            case SpvOpTypeArray:        return SpirvReflection::TypeInfo::ARRAY;
            default:
            {
                u16 opCode = OPCODE(word);
                al_srs_assert(!"Unsupported OpType");
            } break;
        }
        return SpirvReflection::TypeInfo::Kind(0);
    }

    static SpirvReflection::TypeInfo* try_match_scalar_type(SpirvReflection::TypeInfo* targetType, SpirvReflection* reflection)
    {
        SpirvReflection::TypeInfo* alreadySavedValue = reflection->typeInfos;
        for (SpirvReflection::TypeInfo* alreadySavedValue = reflection->typeInfos; alreadySavedValue; alreadySavedValue = alreadySavedValue->next)
        {
            if (alreadySavedValue->kind != targetType->kind)                                    { continue; }
            if (alreadySavedValue->info.scalar.kind != targetType->info.scalar.kind)            { continue; }
            if (alreadySavedValue->info.scalar.bitWidth != targetType->info.scalar.bitWidth)    { continue; }
            if (alreadySavedValue->info.scalar.isSigned != targetType->info.scalar.isSigned)    { continue; }
            return alreadySavedValue;
        }
        return nullptr;
    }

    static SpirvReflection::TypeInfo* try_match_vector_type(SpirvReflection::TypeInfo* targetType, SpirvReflection* reflection)
    {
        SpirvReflection::TypeInfo* alreadySavedValue = reflection->typeInfos;
        for (SpirvReflection::TypeInfo* alreadySavedValue = reflection->typeInfos; alreadySavedValue; alreadySavedValue = alreadySavedValue->next)
        {
            if (alreadySavedValue->kind != targetType->kind)                                                { continue; }
            if (alreadySavedValue->info.vector.componentsCount != targetType->info.vector.componentsCount)  { continue; }
            if (alreadySavedValue->info.vector.componentType != targetType->info.vector.componentType)      { continue; }
            return alreadySavedValue;
        }
        return nullptr;
    }

    static SpirvReflection::TypeInfo* try_match_matrix_type(SpirvReflection::TypeInfo* targetType, SpirvReflection* reflection)
    {
        SpirvReflection::TypeInfo* alreadySavedValue = reflection->typeInfos;
        for (SpirvReflection::TypeInfo* alreadySavedValue = reflection->typeInfos; alreadySavedValue; alreadySavedValue = alreadySavedValue->next)
        {
            if (alreadySavedValue->kind != targetType->kind)                                            { continue; }
            if (alreadySavedValue->info.matrix.columnsCount != targetType->info.matrix.columnsCount)    { continue; }
            if (alreadySavedValue->info.matrix.columnType != targetType->info.matrix.columnType)        { continue; }
            return alreadySavedValue;
        }
        return nullptr;
    }

    static SpirvReflection::TypeInfo* try_match_structure_type(SpirvReflection::TypeInfo* targetType, SpirvReflection* reflection)
    {
        SpirvReflection::TypeInfo* alreadySavedValue = reflection->typeInfos;
        for (SpirvReflection::TypeInfo* alreadySavedValue = reflection->typeInfos; alreadySavedValue; alreadySavedValue = alreadySavedValue->next)
        {
            if (alreadySavedValue->kind != targetType->kind)                                                    { continue; }
            if (std::strcmp(alreadySavedValue->info.structure.typeName, targetType->info.structure.typeName))   { continue; }
            return alreadySavedValue;
        }
        return nullptr;
    }

    static SpirvReflection::TypeInfo* try_match_array_type(SpirvReflection::TypeInfo* targetType, SpirvReflection* reflection)
    {
        SpirvReflection::TypeInfo* alreadySavedValue = reflection->typeInfos;
        for (SpirvReflection::TypeInfo* alreadySavedValue = reflection->typeInfos; alreadySavedValue; alreadySavedValue = alreadySavedValue->next)
        {
            if (alreadySavedValue->kind != targetType->kind)                                    { continue; }
            if (alreadySavedValue->info.array.entryType != targetType->info.array.entryType)    { continue; }
            if (alreadySavedValue->info.array.size != targetType->info.array.size)              { continue; }
            return alreadySavedValue;
        }
        return nullptr;
    }

    static SpirvReflection::TypeInfo* get_last_value_type(SpirvReflection* reflection)
    {
        if (!reflection->typeInfos)
        {
            return nullptr;
        }
        SpirvReflection::TypeInfo* result = reflection->typeInfos;
        while(result->next)
        {
            result = result->next;
        }
        return result;
    }

    static SpirvReflection::TypeInfo* save_or_get_type_from_reflection(SpirvStruct* structs, uSize structsCount, SpirvId* ids, SpirvId* id, SpirvReflection* reflection)
    {
        SpirvReflection::TypeInfo* matchedType = nullptr;
        SpirvReflection::TypeInfo resultType{};
        resultType.kind = op_type_to_value_kind(id->declarationLocation[0]);
        switch(resultType.kind)
        {
            case SpirvReflection::TypeInfo::SCALAR:
            {
                decltype(resultType.info.scalar)* scalar = &resultType.info.scalar;
                switch(OPCODE(id->declarationLocation[0]))
                {
                    case SpvOpTypeFloat:
                    {
                        scalar->kind = SpirvReflection::TypeInfo::ScalarKind::FLOAT;
                        scalar->isSigned = true;
                        scalar->bitWidth = id->declarationLocation[2];
                    } break;
                    case SpvOpTypeInt:
                    {
                        scalar->kind = SpirvReflection::TypeInfo::ScalarKind::INTEGER;
                        scalar->isSigned = id->declarationLocation[3] == 1;
                        scalar->bitWidth = id->declarationLocation[2];
                    } break;
                    default:
                    {
                        al_srs_assert(!"Unsupported scalar value type (bool?)");
                    }
                }
                matchedType = try_match_scalar_type(&resultType, reflection);
            } break;
            case SpirvReflection::TypeInfo::VECTOR:
            {
                decltype(resultType.info.vector)* vector = &resultType.info.vector;
                vector->componentsCount = id->declarationLocation[3];
                vector->componentType = save_or_get_type_from_reflection(structs, structsCount, ids, &ids[id->declarationLocation[2]], reflection);
                al_srs_assert(vector->componentType->kind == SpirvReflection::TypeInfo::SCALAR);
                matchedType = try_match_vector_type(&resultType, reflection);
            } break;
            case SpirvReflection::TypeInfo::MATRIX:
            {
                decltype(resultType.info.matrix)* matrix = &resultType.info.matrix;
                matrix->columnsCount = id->declarationLocation[3];
                matrix->columnType = save_or_get_type_from_reflection(structs, structsCount, ids, &ids[id->declarationLocation[2]], reflection);
                al_srs_assert(matrix->columnType->kind == SpirvReflection::TypeInfo::VECTOR);
                matchedType = try_match_matrix_type(&resultType, reflection);
            } break;
            case SpirvReflection::TypeInfo::STRUCTURE:
            {
                decltype(resultType.info.structure)* structure = &resultType.info.structure;
                SpirvStruct* runtimeStruct = find_struct_by_id(structs, structsCount, id);
                al_srs_assert(runtimeStruct);
                structure->typeName = runtimeStruct->id->name;
                matchedType = try_match_structure_type(&resultType, reflection);
                if (!matchedType)
                {
                    structure->typeName = save_string_to_buffer(structure->typeName, reflection);
                    structure->membersCount = runtimeStruct->membersCount;
                    structure->members = fixed_size_buffer_allocate<SpirvReflection::TypeInfo*>(&reflection->buffer, structure->membersCount);
                    structure->memberNames = fixed_size_buffer_allocate<const char*>(&reflection->buffer, structure->membersCount);
                    for (uSize it = 0; it < structure->membersCount; it++)
                    {
                        structure->members[it] = save_or_get_type_from_reflection(structs, structsCount, ids, runtimeStruct->members[it].id, reflection);
                        structure->memberNames[it] = save_string_to_buffer(runtimeStruct->members[it].name, reflection);
                    }
                }
            } break;
            case SpirvReflection::TypeInfo::ARRAY:
            {
                decltype(resultType.info.array)* array = &resultType.info.array;
                switch(OPCODE(id->declarationLocation[0]))
                {
                    case SpvOpTypeRuntimeArray: { array->size = 0; } break;
                    case SpvOpTypeArray:
                    {
                        SpirvId* constant = &ids[id->declarationLocation[3]];
                        SpirvId* constantType = &ids[constant->declarationLocation[1]];
                        al_srs_assert(OPCODE(constantType->declarationLocation[0]) == SpvOpTypeInt);
                        SpirvWord arraySizeConstantBitWidth = constantType->declarationLocation[2];
                        switch (arraySizeConstantBitWidth)
                        {
                            case 32: array->size = *((u32*)&constant->declarationLocation[3]); break;
                            default: al_srs_assert(!"Unsupported array size constat bit width");
                        };
                    } break;
                    default: al_srs_assert(!"Unknown array type");
                }
                array->entryType = save_or_get_type_from_reflection(structs, structsCount, ids, &ids[id->declarationLocation[2]], reflection);
                matchedType = try_match_array_type(&resultType, reflection);
            } break;
        }
        if (!matchedType)
        {
            matchedType = fixed_size_buffer_allocate<SpirvReflection::TypeInfo>(&reflection->buffer);
            std::memcpy(matchedType, &resultType, sizeof(resultType));
            SpirvReflection::TypeInfo* lastTypeInfo = get_last_value_type(reflection);
            if (lastTypeInfo)   { lastTypeInfo->next = matchedType; }
            else                { reflection->typeInfos = matchedType; }
        }
        return matchedType;
    }

    static void process_shader_io_variable(SpirvStruct* structs, uSize structsCount, SpirvId* ids, SpirvId* ioDeclarationId, SpirvReflection* reflection, SpirvReflection::ShaderIO* io)
    {
        io->name = save_string_to_buffer(ioDeclarationId->name, reflection);
        io->location = ioDeclarationId->location;
        al_srs_assert(OPCODE(ioDeclarationId->declarationLocation[0]) == SpvOpVariable);
        SpirvId* id = &ids[ioDeclarationId->declarationLocation[1]];
        al_srs_assert(OPCODE(id->declarationLocation[0]) == SpvOpTypePointer);
        id = &ids[id->declarationLocation[3]];
        io->typeInfo = save_or_get_type_from_reflection(structs, structsCount, ids, id, reflection);
        if (ioDeclarationId->flags & (1 << SpirvId::IS_BUILT_IN) || id->flags & (1 << SpirvId::IS_BUILT_IN))
        {
            io->flags |= (1 << SpirvReflection::ShaderIO::IS_BUILT_IN);
        }
    }

    static void process_shader_input_variable(SpirvStruct* structs, uSize structsCount, SpirvId* ids, SpirvId* inputVariable, SpirvReflection* reflection)
    {
        SpirvReflection::ShaderIO* input = &reflection->shaderInputs[reflection->shaderInputCount++];
        process_shader_io_variable(structs, structsCount, ids, inputVariable, reflection, input);
    }

    static void process_shader_output_variable(SpirvStruct* structs, uSize structsCount, SpirvId* ids, SpirvId* outputVariable, SpirvReflection* reflection)
    {
        SpirvReflection::ShaderIO* output = &reflection->shaderOutputs[reflection->shaderOutputCount++];
        process_shader_io_variable(structs, structsCount, ids, outputVariable, reflection, output);
    }

    static void process_shader_push_constant(SpirvStruct* structs, uSize structsCount, SpirvId* ids, SpirvId* pushConstant, SpirvReflection* reflection)
    {
        reflection->pushConstant = fixed_size_buffer_allocate<SpirvReflection::PushConstant>(&reflection->buffer);
        reflection->pushConstant->name = save_string_to_buffer(pushConstant->name, reflection);
        // Start from variable declaration
        al_srs_assert(OPCODE(pushConstant->declarationLocation[0]) == SpvOpVariable);
        SpirvId* id = &ids[pushConstant->declarationLocation[1]];
        al_srs_assert(OPCODE(id->declarationLocation[0]) == SpvOpTypePointer);
        al_srs_assert(SpvStorageClass(id->declarationLocation[2]) == SpvStorageClassPushConstant);
        // Go to OpTypeStruct value referenced in OpTypePointer
        id = &ids[id->declarationLocation[3]];
        al_srs_assert(OPCODE(id->declarationLocation[0]) == SpvOpTypeStruct);
        reflection->pushConstant->typeInfo = save_or_get_type_from_reflection(structs, structsCount, ids, id, reflection);
    }

    static void process_shader_uniform_buffer(SpirvStruct* structs, uSize structsCount, SpirvId* ids, SpirvId* uniformId, SpirvReflection* reflection)
    {
        SpirvReflection::Uniform* uniform = &reflection->uniforms[reflection->uniformCount++];
        // Start from variable declaration
        SpirvWord* instruction = uniformId->declarationLocation;
        al_srs_assert(OPCODE(instruction[0]) == SpvOpVariable);
        uniform->name = save_string_to_buffer(uniformId->name, reflection);
        uniform->set = uniformId->set;
        uniform->binding = uniformId->binding;
        // Go to OpTypePointer describing this variable
        instruction = ids[instruction[1]].declarationLocation;
        al_srs_assert(OPCODE(instruction[0]) == SpvOpTypePointer);
        al_srs_assert(SpvStorageClass(instruction[2]) == SpvStorageClassUniform || SpvStorageClass(instruction[2]) == SpvStorageClassUniformConstant);
        // Go to OpTypeStruct or OpTypeSampledImage value referenced in OpTypePointer
        SpirvId* uniformTypeId = &ids[instruction[3]];
        if (OPCODE(uniformTypeId->declarationLocation[0]) == SpvOpTypeStruct)
        {
            if (uniformTypeId->flags & (1 << SpirvId::IS_BLOCK))
            {
                uniform->type = SpirvReflection::Uniform::UNIFORM_BUFFER;
            }
            else
            {
                al_vk_assert(uniformTypeId->flags & (1 << SpirvId::IS_BUFFER_BLOCK));
                uniform->type = SpirvReflection::Uniform::STORAGE_BUFFER;
            }
            uniform->buffer = save_or_get_type_from_reflection(structs, structsCount, ids, uniformTypeId, reflection);
        }
        else if (OPCODE(uniformTypeId->declarationLocation[0]) == SpvOpTypeSampledImage ||  // Sampler1D, Sampler2D, Sampler3D
                 OPCODE(uniformTypeId->declarationLocation[0]) == SpvOpTypeImage)           // subpassInput
        {
            uniform->type = SpirvReflection::Uniform::IMAGE;
            std::memset(&uniform->image, 0, sizeof(uniform->image));
            if (OPCODE(uniformTypeId->declarationLocation[0]) == SpvOpTypeSampledImage)
            {
                uniform->image.flags |= 1 << SpirvReflection::Image::IS_SAMPLED;
            }
            else
            {
                uniform->image.flags |= 1 << SpirvReflection::Image::IS_SUBPASS_DEPENDENCY;
            }
        }
        else
        {
            al_srs_assert(!"Unsupported uniform buffer type");
        }
    }

    static u32 get_storage_class_flags(SpirvWord word)
    {
        SpvStorageClass storageClass = SpvStorageClass(word);
        switch (storageClass)
        {
            case SpvStorageClassInput:              return 1 << SpirvId::IS_INPUT;
            case SpvStorageClassOutput:             return 1 << SpirvId::IS_OUTPUT;
            case SpvStorageClassPushConstant:       return 1 << SpirvId::IS_PUSH_CONSTANT;
            case SpvStorageClassUniform:            return 1 << SpirvId::IS_UNIFORM;
            case SpvStorageClassUniformConstant:    return 1 << SpirvId::IS_UNIFORM;
        }
        return 0;
    }

    void construct_spirv_reflection(SpirvReflection* reflection, AllocatorBindings* bindings, SpirvWord* bytecode, uSize wordCount)
    {
        al_srs_assert(bytecode);
        std::memset(reflection, 0, sizeof(SpirvReflection));
        //
        // Basic header validation + retrieving number of unique ids (bound) used in bytecode
        //
        al_srs_assert(bytecode[0] == SpvMagicNumber);
        uSize bound = bytecode[3];
        SpirvWord* instruction = nullptr;
        //
        // Allocate SpirvId array which will hols all neccessary information about shaders identifiers
        //
        SpirvId* ids = nullptr;
        ids = (SpirvId*)allocate(bindings, sizeof(SpirvId) * bound);
        std::memset(ids, 0, sizeof(SpirvId) * bound);
        defer(deallocate(bindings, ids, sizeof(SpirvId) * bound));
        //
        // Step 1. Filling ids array + some general shader info
        //
#define SAVE_DECLARATION_LOCATION(index) { SpirvWord id = instruction[index]; al_srs_assert(id < bound); ids[id].declarationLocation = instruction; }
        uSize inputsCount = 0;
        uSize outputsCount = 0;
        uSize uniformsCount = 0;
        for (instruction = bytecode + 5; instruction < (bytecode + wordCount);)
        {
            u16 instructionOpCode = OPCODE(instruction[0]);
            u16 instructionWordCount = WORDCOUNT(instruction[0]);
            switch (instructionOpCode)
            {
                case SpvOpEntryPoint:
                {
                    al_srs_assert(instructionWordCount >= 4);
                    reflection->shaderType = execution_mode_to_shader_type(SpvExecutionModel(instruction[1]));
                    reflection->entryPointName = save_string_to_buffer((char*)&instruction[3], reflection);
                } break;
                case SpvOpName:
                {
                    al_srs_assert(instructionWordCount >= 3);
                    ids[instruction[1]].name = (char*)&instruction[2];
                } break;
                case SpvOpTypeFloat:    al_srs_assert(instructionWordCount == 3); SAVE_DECLARATION_LOCATION(1); break;
                case SpvOpTypeInt:      al_srs_assert(instructionWordCount == 4); SAVE_DECLARATION_LOCATION(1); break;
                case SpvOpTypeVector:   al_srs_assert(instructionWordCount == 4); SAVE_DECLARATION_LOCATION(1); break;
                case SpvOpTypeMatrix:   al_srs_assert(instructionWordCount == 4); SAVE_DECLARATION_LOCATION(1); break;
                case SpvOpTypeStruct:   al_srs_assert(instructionWordCount >= 2); SAVE_DECLARATION_LOCATION(1); break;
                case SpvOpConstant:     al_srs_assert(instructionWordCount >= 3); SAVE_DECLARATION_LOCATION(2); break;
                case SpvOpTypePointer:
                {
                    al_srs_assert(instructionWordCount == 4);
                    SAVE_DECLARATION_LOCATION(1);
                    ids[instruction[1]].flags |= get_storage_class_flags(instruction[2]);
                } break;
                case SpvOpVariable:
                {
                    al_srs_assert(instructionWordCount >= 4);
                    SAVE_DECLARATION_LOCATION(2);
                    ids[instruction[2]].flags |= get_storage_class_flags(instruction[3]);
                    if (ids[instruction[2]].flags & (1 << SpirvId::IS_INPUT))   inputsCount += 1;
                    if (ids[instruction[2]].flags & (1 << SpirvId::IS_OUTPUT))  outputsCount += 1;
                    if (ids[instruction[2]].flags & (1 << SpirvId::IS_UNIFORM)) uniformsCount += 1;
                } break;
                case SpvOpTypeSampledImage:
                {
                    al_srs_assert(instructionWordCount == 3);
                    SAVE_DECLARATION_LOCATION(1);
                } break;
                case SpvOpTypeImage:
                {
                    al_srs_assert(instructionWordCount >= 9);
                    SAVE_DECLARATION_LOCATION(1);
                } break;
                case SpvOpTypeRuntimeArray:
                {
                    al_srs_assert(instructionWordCount == 3);
                    SAVE_DECLARATION_LOCATION(1);
                } break;
                case SpvOpTypeArray:
                {
                    al_srs_assert(instructionWordCount == 4);
                    SAVE_DECLARATION_LOCATION(1);
                } break;
                case SpvOpMemberDecorate:
                {
                    al_srs_assert(instructionWordCount >= 4);
                    SpvDecoration decoration = SpvDecoration(instruction[3]);
                    switch (decoration)
                    {
                        case SpvDecorationBuiltIn:
                        {
                            // Here we basically saying that if structure member has "BuiltIn" decoration,
                            // then the whole structure is considered "BuiltIn" and will be ignored in reflection
                            al_srs_assert(instructionWordCount == 5);
                            SpirvWord id = instruction[1];
                            al_srs_assert(id < bound);
                            ids[id].flags |= 1 << SpirvId::IS_BUILT_IN;
                        } break;
                    }
                } break;
                case SpvOpDecorate:
                {
                    al_srs_assert(instructionWordCount >= 3);
                    SpvDecoration decoration = SpvDecoration(instruction[2]);
                    switch (decoration)
                    {
                        case SpvDecorationBlock:
                        {
                            al_srs_assert(instructionWordCount == 3);
                            SpirvWord id = instruction[1];
                            al_srs_assert(id < bound);
                            ids[id].flags |= 1 << SpirvId::IS_BLOCK;
                        } break;
                        case SpvDecorationBufferBlock:
                        {
                            al_srs_assert(instructionWordCount == 3);
                            SpirvWord id = instruction[1];
                            al_srs_assert(id < bound);
                            ids[id].flags |= 1 << SpirvId::IS_BUFFER_BLOCK;
                        } break;
                        case SpvDecorationBuiltIn:
                        {
                            al_srs_assert(instructionWordCount == 4);
                            SpirvWord id = instruction[1];
                            al_srs_assert(id < bound);
                            ids[id].flags |= 1 << SpirvId::IS_BUILT_IN;
                        } break;
                        case SpvDecorationLocation:
                        {
                            al_srs_assert(instructionWordCount == 4);
                            SpirvWord id = instruction[1];
                            al_srs_assert(id < bound);
                            ids[id].location = decltype(ids[id].location)(instruction[3]);
                            ids[id].flags |= 1 << SpirvId::HAS_LOCATION_DECORATION;
                        } break;
                        case SpvDecorationBinding:
                        {
                            al_srs_assert(instructionWordCount == 4);
                            SpirvWord id = instruction[1];
                            al_srs_assert(id < bound);
                            ids[id].binding = decltype(ids[id].binding)(instruction[3]);
                            ids[id].flags |= 1 << SpirvId::HAS_BINDING_DECORATION;
                        } break;
                        case SpvDecorationDescriptorSet:
                        {
                            al_srs_assert(instructionWordCount == 4);
                            SpirvWord id = instruction[1];
                            al_srs_assert(id < bound);
                            ids[id].set = decltype(ids[id].set)(instruction[3]);
                            ids[id].flags |= 1 << SpirvId::HAS_SET_DECORATION;
                        } break;
                    }
                } break;
            }
            instruction += instructionWordCount;
        }
        reflection->shaderInputs = fixed_size_buffer_allocate<SpirvReflection::ShaderIO>(&reflection->buffer, inputsCount);
        reflection->shaderOutputs = fixed_size_buffer_allocate<SpirvReflection::ShaderIO>(&reflection->buffer, outputsCount);
        reflection->uniforms = fixed_size_buffer_allocate<SpirvReflection::Uniform>(&reflection->buffer, uniformsCount);
#undef SAVE_DECLARATION_LOCATION
        //
        // Step 2. Retrieving information about shader structs
        //
        //
        // Step 2.1. Count number of structs and struct members
        //
        SpirvStruct* structs;
        SpirvStruct::Member* structMembers;
        uSize structsCount = 0;
        uSize structmembersCount = 0;
        for (instruction = bytecode + 5; instruction < (bytecode + wordCount);)
        {
            u16 instructionOpCode = OPCODE(instruction[0]);
            u16 instructionWordCount = WORDCOUNT(instruction[0]);
            switch (instructionOpCode)
            {
                case SpvOpTypeStruct:
                {
                    structsCount += 1;
                    structmembersCount += instructionWordCount - 2;
                } break;
            }
            instruction += instructionWordCount;
        }
        //
        // Step 2.2. Allocate structs and struct members arrays
        //
        structs = (SpirvStruct*)allocate(bindings, sizeof(SpirvStruct) * structsCount);
        defer(deallocate(bindings, structs, sizeof(SpirvStruct) * structsCount));
        structMembers = (SpirvStruct::Member*)allocate(bindings, sizeof(SpirvStruct::Member) * structmembersCount);
        defer(deallocate(bindings, structMembers, sizeof(SpirvStruct::Member) * structmembersCount));
        //
        // Step 2.3. Save struct and struct members ids
        //
        uSize structCounter = 0;
        uSize memberCounter = 0;
        for (instruction = bytecode + 5; instruction < (bytecode + wordCount);)
        {
            u16 instructionOpCode = OPCODE(instruction[0]);
            u16 instructionWordCount = WORDCOUNT(instruction[0]);
            switch (instructionOpCode)
            {
                case SpvOpTypeStruct:
                {
                    SpirvStruct* shaderStruct = &structs[structCounter++];
                    shaderStruct->id = &ids[instruction[1]];
                    shaderStruct->membersCount = instructionWordCount - 2;
                    shaderStruct->members = &structMembers[memberCounter];
                    memberCounter += shaderStruct->membersCount;
                    for (uSize it = 0; it < shaderStruct->membersCount; it++)
                    {
                        SpirvStruct::Member* member = &shaderStruct->members[it];
                        member->id = &ids[instruction[2 + it]];
                    }
                } break;
            }
            instruction += instructionWordCount;
        }
        //
        // Step 2.4. Save struct members names
        //
        for (instruction = bytecode + 5; instruction < (bytecode + wordCount);)
        {
            u16 instructionOpCode = OPCODE(instruction[0]);
            u16 instructionWordCount = WORDCOUNT(instruction[0]);
            switch (instructionOpCode)
            {
                case SpvOpMemberName:
                {
                    SpirvId* targetStructId = &ids[instruction[1]];
                    for (uSize it = 0; it < structsCount; it++)
                    {
                        SpirvStruct* shaderStruct = &structs[it];
                        if (shaderStruct->id != targetStructId)
                        {
                            continue;
                        }
                        SpirvStruct::Member* member = &shaderStruct->members[instruction[2]];
                        member->name = (char*)&instruction[3];
                    }
                } break;
            }
            instruction += instructionWordCount;
        }
        //
        // Step 3. Process all input and ouptut variables, push constants and buffers
        //
        for (uSize it = 0; it < bound; it++)
        {
            SpirvId* id = &ids[it];
            if (it == 53)
            {
                int a = 0;
            }
            if (id->declarationLocation && OPCODE(id->declarationLocation[0]) == SpvOpVariable)
            {
                if (id->flags & (1 << SpirvId::IS_INPUT))
                {
                    process_shader_input_variable(structs, structsCount, ids, id, reflection);
                }
                else if (id->flags & (1 << SpirvId::IS_OUTPUT))
                {
                    process_shader_output_variable(structs, structsCount, ids, id, reflection);
                }
                else if (id->flags & (1 << SpirvId::IS_PUSH_CONSTANT))
                {
                    process_shader_push_constant(structs, structsCount, ids, id, reflection);
                }
                else if (id->flags & (1 << SpirvId::IS_UNIFORM))
                {
                    process_shader_uniform_buffer(structs, structsCount, ids, id, reflection);
                }
            }
        }
    }

    void print_type_info(SpirvReflection::TypeInfo* typeInfo, SpirvReflection* reflection, uSize indentationLevel, const char* structMemberName)
    {
        static const char* INDENTATIONS[] =
        {
            "",
            al_srs_indent,
            al_srs_indent al_srs_indent,
            al_srs_indent al_srs_indent al_srs_indent,
            al_srs_indent al_srs_indent al_srs_indent al_srs_indent,
            al_srs_indent al_srs_indent al_srs_indent al_srs_indent al_srs_indent,
            al_srs_indent al_srs_indent al_srs_indent al_srs_indent al_srs_indent al_srs_indent,
            al_srs_indent al_srs_indent al_srs_indent al_srs_indent al_srs_indent al_srs_indent al_srs_indent,
            al_srs_indent al_srs_indent al_srs_indent al_srs_indent al_srs_indent al_srs_indent al_srs_indent al_srs_indent,
            al_srs_indent al_srs_indent al_srs_indent al_srs_indent al_srs_indent al_srs_indent al_srs_indent al_srs_indent al_srs_indent,
            al_srs_indent al_srs_indent al_srs_indent al_srs_indent al_srs_indent al_srs_indent al_srs_indent al_srs_indent al_srs_indent al_srs_indent,
        };
        uSize clampedIndentationLevel = indentationLevel < al_srs_array_size(INDENTATIONS) ? indentationLevel : al_srs_array_size(INDENTATIONS) - 1;
        al_srs_printf("%s", INDENTATIONS[clampedIndentationLevel]);
        if (structMemberName)
        {
            al_srs_printf("%s: ", structMemberName);
        }
        switch(typeInfo->kind)
        {
            case SpirvReflection::TypeInfo::SCALAR:
            {
                static const char* SCALAR_KINDS[] = { "int", "float" };
                al_srs_printf("%s ", typeInfo->info.scalar.isSigned ? "signed" : "unsigned");
                al_srs_printf("%s", SCALAR_KINDS[static_cast<uSize>(typeInfo->info.scalar.kind)]);
                al_srs_printf("%d.\n", typeInfo->info.scalar.bitWidth);
            } break;
            case SpirvReflection::TypeInfo::VECTOR:
            {
                al_srs_printf("vec%d of type ", typeInfo->info.vector.componentsCount);
                print_type_info(typeInfo->info.vector.componentType, reflection, 0);
            } break;
            case SpirvReflection::TypeInfo::MATRIX:
            {
                al_srs_printf("mat%d with column type ", typeInfo->info.matrix.columnsCount);
                print_type_info(typeInfo->info.matrix.columnType, reflection, 0);
            } break;
            case SpirvReflection::TypeInfo::STRUCTURE:
            {
                al_srs_printf("struct %s with following members:\n", typeInfo->info.structure.typeName);
                for (uSize it = 0; it < typeInfo->info.structure.membersCount; it++)
                {
                    print_type_info(typeInfo->info.structure.members[it], reflection, indentationLevel + 1, typeInfo->info.structure.memberNames[it]);
                }
            } break;
            case SpirvReflection::TypeInfo::ARRAY:
            {
                if (typeInfo->info.array.size == 0) al_srs_printf("runtime array ");
                else                                al_srs_printf("array of size %zd ", typeInfo->info.array.size);
                al_srs_printf("with following entry type:\n");
                print_type_info(typeInfo->info.array.entryType, reflection, indentationLevel + 1);
            } break;
            default: al_srs_assert(!"Unknown SpirvReflection::TypeInfo::Kind");
        }
    }

    uSize get_type_info_size(SpirvReflection::TypeInfo* typeInfo)
    {
        switch(typeInfo->kind)
        {
            case SpirvReflection::TypeInfo::SCALAR:
            {
                return typeInfo->info.scalar.bitWidth / 8;
            } break;
            case SpirvReflection::TypeInfo::VECTOR:
            {
                return typeInfo->info.vector.componentsCount * get_type_info_size(typeInfo->info.vector.componentType);
            } break;
            case SpirvReflection::TypeInfo::MATRIX:
            {
                return typeInfo->info.matrix.columnsCount * get_type_info_size(typeInfo->info.matrix.columnType);
            } break;
            case SpirvReflection::TypeInfo::STRUCTURE:
            {
                uSize resultSize = 0;
                for (uSize it = 0; it < typeInfo->info.structure.membersCount; it++)
                {
                    resultSize += get_type_info_size(typeInfo->info.structure.members[it]);
                }
                return resultSize;
            } break;
            case SpirvReflection::TypeInfo::ARRAY:
            {
                return typeInfo->info.array.size * get_type_info_size(typeInfo->info.array.entryType);
            } break;
            default: al_srs_assert(!"Unknown SpirvReflection::TypeInfo::Kind");
        }
        return 0;
    }

    const char* shader_type_to_str(SpirvReflection::ShaderType type)
    {
#define CASE(code) case code: return #code
        switch (type)
        {
            CASE(SpirvReflection::VERTEX);
            CASE(SpirvReflection::FRAGMENT);
        }
        al_srs_assert(!"Unsupported SpirvReflection::ShaderType value");
        return "";
#undef CASE
    }
}

#undef OPCODE
#undef WORDCOUNT
