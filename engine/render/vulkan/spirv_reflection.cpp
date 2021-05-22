
#include <cstring>

#include "spirv_reflection.h"

#define OPCODE(word) u16(word)
#define WORDCOUNT(word) u16(instruction[0] >> 16)

namespace al::vulkan
{
    SpirvReflection::ShaderType execution_mode_to_shader_type(SpvExecutionModel model)
    {
        switch (model)
        {
            case SpvExecutionModelVertex    : return SpirvReflection::VERTEX;
            case SpvExecutionModelFragment  : return SpirvReflection::FRAGMENT;
        }
        al_vk_assert(!"Unsupported SpvExecutionModel value");
        return SpirvReflection::ShaderType(0);
    }

    const char* op_code_to_str(u16 opCode)
    {
#define CASE(code) case code: return #code
        switch (opCode)
        {
            CASE(SpvOpTypePointer);
            CASE(SpvOpVariable);
        }
        al_vk_assert(!"Unsupported opCode value");
        return "";
#undef CASE
    }

    u8* allocate_data_in_buffer(uSize dataSize, u8* buffer, uSize* bufferPtr, uSize bufferSize)
    {
        u8* result = nullptr;
        al_vk_assert(dataSize < (bufferSize - *bufferPtr) && "Dynamic buffer overflow");
        result = &buffer[*bufferPtr];
        *bufferPtr += dataSize;
        return result;
    }

    u8* allocate_data_in_buffer(uSize dataSize, SpirvReflection* reflection)
    {
        return allocate_data_in_buffer(dataSize, reflection->dynamicMemoryBuffer, &reflection->dynamicMemoryBufferCounter, SpirvReflection::DYNAMIC_BUFFER_BUFFER_SIZE);
    }

    u8* save_data_to_buffer(void* data, uSize dataSize, u8* buffer, uSize* bufferPtr, uSize bufferSize)
    {
        u8* result = allocate_data_in_buffer(dataSize, buffer, bufferPtr, bufferSize);
        std::memcpy(result, data, dataSize);
        return result;
    }

    char* save_string_to_buffer(char* str, u8* buffer, uSize* bufferPtr, uSize bufferSize)
    {
        uSize length = std::strlen(str);
        char* result = (char*)save_data_to_buffer(str, length + 1, buffer, bufferPtr, bufferSize);
        result[length] = 0;
        return result;
    }

    char* save_string_to_buffer(char* str, SpirvReflection* reflection)
    {
        return save_string_to_buffer(str, reflection->dynamicMemoryBuffer, &reflection->dynamicMemoryBufferCounter, SpirvReflection::DYNAMIC_BUFFER_BUFFER_SIZE);
    }

    uSize get_id_size(SpirvStruct* structs, uSize numStructs, SpirvId* ids, SpirvId* id)
    {
        switch (OPCODE(id->declarationLocation[0]))
        {
            case SpvOpTypeFloat:
            {
                return id->declarationLocation[2];
            } break;
            case SpvOpTypeInt:
            {
                return id->declarationLocation[2];
            } break;
            case SpvOpTypeVector:
            {
                SpirvWord componentsNum = id->declarationLocation[3];
                SpirvWord singleComponentSize = get_id_size(structs, numStructs, ids, &ids[id->declarationLocation[2]]);
                return componentsNum * singleComponentSize;
            } break;
            case SpvOpTypeMatrix:
            {
                SpirvWord columnsNum = id->declarationLocation[3];
                SpirvWord columnSize = get_id_size(structs, numStructs, ids, &ids[id->declarationLocation[2]]);
                return columnsNum * columnSize;
            } break;
            case SpvOpTypeStruct:
            {
                SpirvStruct* shaderStruct = find_struct_by_id(structs, numStructs, id);
                uSize resultSize = 0;
                for (uSize memberIt = 0; memberIt < shaderStruct->membersNum; memberIt++)
                {
                    SpirvStruct::Member* member = &shaderStruct->members[memberIt];
                    resultSize += get_id_size(structs, numStructs, ids, member->id);
                }
                return resultSize;
            } break;
            default:
            {
                al_vk_assert(!"Unsupported OpType");
            } break;
        }
        return 0;
    }

    SpirvStruct* find_struct_by_id(SpirvStruct* structs, uSize numStructs, SpirvId* id)
    {
        for (uSize it = 0; it < numStructs; it++)
        {
            if (structs[it].id == id)
            {
                return &structs[it];
            }
        }
        return nullptr;
    }

    void save_runtime_struct_to_result_struct(SpirvStruct* structs, uSize structsNum, SpirvId* ids, SpirvStruct* runtimeStruct, SpirvReflection::ShaderStruct* resultStruct, SpirvReflection* reflection)
    {
        resultStruct->name = save_string_to_buffer(runtimeStruct->id->name, reflection);
        resultStruct->membersNum = runtimeStruct->membersNum;
        resultStruct->members = (SpirvReflection::ShaderStruct::Member*)allocate_data_in_buffer(sizeof(SpirvReflection::ShaderStruct::Member) * runtimeStruct->membersNum, reflection);
        for (uSize membersIt = 0; membersIt < runtimeStruct->membersNum; membersIt++)
        {
            SpirvReflection::ShaderStruct::Member* member = &resultStruct->members[membersIt];
            member->name = save_string_to_buffer(runtimeStruct->members[membersIt].name, reflection);
            member->sizeBytes = get_id_size(structs, structsNum, ids, runtimeStruct->members[membersIt].id);
        }
    }

    void process_shader_input_variable(SpirvStruct* structs, uSize structsNum, SpirvId* ids, SpirvId* inputVariable, SpirvReflection* reflection)
    {
        SpirvReflection::ShaderInput* input = &reflection->shaderInputs[reflection->shaderInputCount++];
        input->name = save_string_to_buffer(inputVariable->name, reflection);
        input->location = inputVariable->location;
        {   // Getting size of variable
            // Start from variable declaration
            SpirvWord* instruction = inputVariable->declarationLocation;
            al_vk_assert(OPCODE(instruction[0]) == SpvOpVariable);
            // Go to OpTypePointer describing this variable
            instruction = ids[instruction[1]].declarationLocation;
            al_vk_assert(OPCODE(instruction[0]) == SpvOpTypePointer);
            al_vk_assert(SpvStorageClass(instruction[2]) == SpvStorageClassInput);
            // Go to whatever OpType... value referenced in OpTypePointer
            input->sizeBytes = get_id_size(structs, structsNum, ids, &ids[instruction[3]]);
        }
    }

    void process_shader_push_constant(SpirvStruct* structs, uSize structsNum, SpirvId* ids, SpirvId* pushConstant, SpirvReflection* reflection)
    {
        reflection->pushConstant = (SpirvReflection::ShaderStruct*)allocate_data_in_buffer(sizeof(SpirvReflection::ShaderStruct), reflection);
        // Start from variable declaration
        SpirvWord* instruction = pushConstant->declarationLocation;
        al_vk_assert(OPCODE(instruction[0]) == SpvOpVariable);
        // Go to OpTypePointer describing this variable
        instruction = ids[instruction[1]].declarationLocation;
        al_vk_assert(OPCODE(instruction[0]) == SpvOpTypePointer);
        al_vk_assert(SpvStorageClass(instruction[2]) == SpvStorageClassPushConstant);
        // Go to OpTypeStruct value referenced in OpTypePointer
        SpirvId* structId = &ids[instruction[3]];
        al_vk_assert(OPCODE(structId->declarationLocation[0]) == SpvOpTypeStruct);
        SpirvStruct* shaderStruct = find_struct_by_id(structs, structsNum, structId);
        save_runtime_struct_to_result_struct(structs, structsNum, ids, shaderStruct, reflection->pushConstant, reflection);
    }

    void process_shader_uniform_buffer(SpirvStruct* structs, uSize structsNum, SpirvId* ids, SpirvId* uniformId, SpirvReflection* reflection)
    {
        SpirvReflection::Uniform* uniform = &reflection->uniforms[reflection->uniformCount++];
        // Start from variable declaration
        SpirvWord* instruction = uniformId->declarationLocation;
        al_vk_assert(OPCODE(instruction[0]) == SpvOpVariable);
        // Go to OpTypePointer describing this variable
        instruction = ids[instruction[1]].declarationLocation;
        al_vk_assert(OPCODE(instruction[0]) == SpvOpTypePointer);
        al_vk_assert(SpvStorageClass(instruction[2]) == SpvStorageClassUniform || SpvStorageClass(instruction[2]) == SpvStorageClassUniformConstant);
        // Go to OpTypeStruct or OpTypeSampledImage value referenced in OpTypePointer
        SpirvId* uniformTypeId = &ids[instruction[3]];
        uniform->set = uniformId->set;
        uniform->binding = uniformId->binding;
        if (OPCODE(uniformTypeId->declarationLocation[0]) == SpvOpTypeStruct)
        {
            uniform->type = SpirvReflection::Uniform::BUFFER;
            uniform->buffer = (SpirvReflection::ShaderStruct*)allocate_data_in_buffer(sizeof(SpirvReflection::ShaderStruct), reflection);
            SpirvStruct* shaderStruct = find_struct_by_id(structs, structsNum, uniformTypeId);
            save_runtime_struct_to_result_struct(structs, structsNum, ids, shaderStruct, uniform->buffer, reflection);
        }
        else if (OPCODE(uniformTypeId->declarationLocation[0]) == SpvOpTypeSampledImage ||  // Sampler1D, Sampler2D, Sampler3D
                 OPCODE(uniformTypeId->declarationLocation[0]) == SpvOpTypeImage)           // subpassInput
        {
            uniform->type = SpirvReflection::Uniform::IMAGE;
            uniform->image = (SpirvReflection::Image*)allocate_data_in_buffer(sizeof(SpirvReflection::Image), reflection);
            if (OPCODE(uniformTypeId->declarationLocation[0]) == SpvOpTypeSampledImage)
            {
                uniform->image->flags |= 1 << SpirvReflection::Image::IS_SAMPLED;
            }
            else
            {
                uniform->image->flags |= 1 << SpirvReflection::Image::IS_SUBPASS_DEPENDENCY;
            }
        }
        else
        {
            al_vk_assert(!"Unsupported uniform buffer type");
        }
    }

    u32 get_storage_class_flags(SpirvWord word)
    {
        SpvStorageClass storageClass = SpvStorageClass(word);
        switch (storageClass)
        {
            case SpvStorageClassInput:
            {
                return 1 << SpirvId::IS_INPUT;
            } break;
            case SpvStorageClassOutput:
            {
                return 1 << SpirvId::IS_OUTPUT;
            } break;
            case SpvStorageClassPushConstant:
            {
                return 1 << SpirvId::IS_PUSH_CONSTANT;
            } break;
            case SpvStorageClassUniform:
            case SpvStorageClassUniformConstant:
            {
                // Is it correct not to make destinction between Unform and UniformConstant?
                return 1 << SpirvId::IS_UNIFORM;
            } break;
        }
        return 0;
    }

    void construct_spirv_reflecttion(SpirvReflection* reflection, AllocatorBindings bindings, SpirvWord* bytecode, uSize wordCount)
    {
        al_vk_assert(bytecode);
        std::memset(reflection, 0, sizeof(SpirvReflection));
        //
        // Header validation and info
        //
        al_vk_assert(bytecode[0] == SpvMagicNumber);
        uSize bound = bytecode[3];
        SpirvWord* instruction = nullptr;
        SpirvId* ids = nullptr;
        ids = (SpirvId*)allocate(&bindings, sizeof(SpirvId) * bound);
        std::memset(ids, 0, sizeof(SpirvId) * bound);
        defer(deallocate(&bindings, ids, sizeof(SpirvId) * bound));
        //
        // Step 1. Filling ids array + some general shader info
        //
#define SAVE_DECLARATION_LOCATION(index) { SpirvWord id = instruction[index]; al_vk_assert(id < bound); ids[id].declarationLocation = instruction; }
        uSize inputsNum = 0;
        uSize uniformsNum = 0;
        for (instruction = bytecode + 5; instruction < (bytecode + wordCount);)
        {
            u16 instructionOpCode = OPCODE(instruction[0]);
            u16 instructionWordCount = WORDCOUNT(instruction[0]);
            switch (instructionOpCode)
            {
                case SpvOpEntryPoint:
                {
                    al_vk_assert(instructionWordCount >= 4);
                    reflection->shaderType = execution_mode_to_shader_type(SpvExecutionModel(instruction[1]));
                    reflection->entryPointName = save_string_to_buffer((char*)&instruction[3], reflection);
                } break;
                case SpvOpName:
                {
                    al_vk_assert(instructionWordCount >= 3);
                    ids[instruction[1]].name = (char*)&instruction[2];
                } break;
                case SpvOpTypeFloat:    al_vk_assert(instructionWordCount == 3); SAVE_DECLARATION_LOCATION(1); break;
                case SpvOpTypeInt:      al_vk_assert(instructionWordCount == 4); SAVE_DECLARATION_LOCATION(1); break;
                case SpvOpTypeVector:   al_vk_assert(instructionWordCount == 4); SAVE_DECLARATION_LOCATION(1); break;
                case SpvOpTypeMatrix:   al_vk_assert(instructionWordCount == 4); SAVE_DECLARATION_LOCATION(1); break;
                case SpvOpTypeStruct:   al_vk_assert(instructionWordCount >= 2); SAVE_DECLARATION_LOCATION(1); break;
                case SpvOpTypePointer:
                {
                    al_vk_assert(instructionWordCount == 4);
                    SAVE_DECLARATION_LOCATION(1);
                    ids[instruction[1]].flags |= get_storage_class_flags(instruction[2]);
                } break;
                case SpvOpVariable:
                {
                    al_vk_assert(instructionWordCount >= 4);
                    SAVE_DECLARATION_LOCATION(2);
                    ids[instruction[2]].flags |= get_storage_class_flags(instruction[3]);
                    if (ids[instruction[2]].flags & (1 << SpirvId::IS_INPUT))
                    {
                        inputsNum += 1;
                    }
                    if (ids[instruction[2]].flags & (1 << SpirvId::IS_UNIFORM))
                    {
                        uniformsNum += 1;
                    }
                } break;
                case SpvOpTypeSampledImage:
                {
                    al_vk_assert(instructionWordCount == 3);
                    SAVE_DECLARATION_LOCATION(1);
                } break;
                case SpvOpTypeImage:
                {
                    al_vk_assert(instructionWordCount >= 9);
                    SAVE_DECLARATION_LOCATION(1);
                } break;
                case SpvOpDecorate:
                {
                    al_vk_assert(instructionWordCount >= 3);
                    SpvDecoration decoration = SpvDecoration(instruction[2]);
                    switch (decoration)
                    {
                        case SpvDecorationLocation:
                        {
                            al_vk_assert(instructionWordCount == 4);
                            SpirvWord id = instruction[1];
                            al_vk_assert(id < bound);
                            ids[id].location = decltype(ids[id].location)(instruction[3]);
                            ids[id].flags |= 1 << SpirvId::HAS_LOCATION_DECORATION;
                        } break;
                        case SpvDecorationBinding:
                        {
                            al_vk_assert(instructionWordCount == 4);
                            SpirvWord id = instruction[1];
                            al_vk_assert(id < bound);
                            ids[id].binding = decltype(ids[id].binding)(instruction[3]);
                            ids[id].flags |= 1 << SpirvId::HAS_BINDING_DECORATION;
                        } break;
                        case SpvDecorationDescriptorSet:
                        {
                            al_vk_assert(instructionWordCount == 4);
                            SpirvWord id = instruction[1];
                            al_vk_assert(id < bound);
                            ids[id].set = decltype(ids[id].set)(instruction[3]);
                            ids[id].flags |= 1 << SpirvId::HAS_SET_DECORATION;
                        } break;
                    }
                } break;
            }
            instruction += instructionWordCount;
        }
        reflection->shaderInputs = (SpirvReflection::ShaderInput*)allocate_data_in_buffer(sizeof(SpirvReflection::ShaderInput) * inputsNum, reflection);
        reflection->uniforms = (SpirvReflection::Uniform*)allocate_data_in_buffer(sizeof(SpirvReflection::Uniform) * uniformsNum, reflection);
#undef SAVE_DECLARATION_LOCATION
        //
        // Step 2. Retrieving information about shader structs
        //
        SpirvStruct* structs;
        SpirvStruct::Member* structMembers;
        uSize structsNum = 0;
        uSize structMembersNum = 0;
        for (instruction = bytecode + 5; instruction < (bytecode + wordCount);)
        {
            u16 instructionOpCode = OPCODE(instruction[0]);
            u16 instructionWordCount = WORDCOUNT(instruction[0]);
            switch (instructionOpCode)
            {
                case SpvOpTypeStruct:
                {
                    structsNum += 1;
                    structMembersNum += instructionWordCount - 2;
                } break;
            }
            instruction += instructionWordCount;
        }
        structs = (SpirvStruct*)allocate(&bindings, sizeof(SpirvStruct) * structsNum);
        defer(deallocate(&bindings, structs, sizeof(SpirvStruct) * structsNum));
        structMembers = (SpirvStruct::Member*)allocate(&bindings, sizeof(SpirvStruct::Member) * structMembersNum);
        defer(deallocate(&bindings, structMembers, sizeof(SpirvStruct::Member) * structMembersNum));
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
                    shaderStruct->membersNum = instructionWordCount - 2;
                    shaderStruct->members = &structMembers[memberCounter];
                    memberCounter += shaderStruct->membersNum;
                    for (uSize it = 0; it < shaderStruct->membersNum; it++)
                    {
                        SpirvStruct::Member* member = &shaderStruct->members[it];
                        SpirvWord* memberDeclaration = ids[instruction[2 + it]].declarationLocation;
                        member->id = &ids[instruction[2 + it]];
                    }
                } break;
            }
            instruction += instructionWordCount;
        }
        for (instruction = bytecode + 5; instruction < (bytecode + wordCount);)
        {
            u16 instructionOpCode = OPCODE(instruction[0]);
            u16 instructionWordCount = WORDCOUNT(instruction[0]);
            switch (instructionOpCode)
            {
                case SpvOpMemberName:
                {
                    SpirvId* targetStructId = &ids[instruction[1]];
                    for (uSize it = 0; it < structsNum; it++)
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
        printf("=============================================================\n");
        printf(" SHADER STRUCTS\n");
        printf("=============================================================\n");
        for (uSize structIt = 0; structIt < structsNum; structIt++)
        {
            SpirvStruct* shaderStruct = &structs[structIt];
            printf("    Shader struct number %d :\n", (int)structIt);
            if (shaderStruct->id->name)
            {
                printf("        Struct name : %s\n", shaderStruct->id->name);
            }
            printf("        Members num : %d\n", (int)shaderStruct->membersNum);
            for (uSize memberIt = 0; memberIt < shaderStruct->membersNum; memberIt++)
            {
                SpirvStruct::Member* member = &shaderStruct->members[memberIt];
                printf("            Member %d name : %s\n", (int)memberIt, member->name);
            }
        }
        //
        // Step 3. Process all input and ouptut variables, push constants and buffers
        //
        for (uSize it = 0; it < bound; it++)
        {
            SpirvId* id = &ids[it];
            if (id->declarationLocation && OPCODE(id->declarationLocation[0]) == SpvOpVariable)
            {
                if (id->flags & (1 << SpirvId::IS_INPUT))
                {
                    process_shader_input_variable(structs, structsNum, ids, id, reflection);
                }
                else if (id->flags & (1 << SpirvId::IS_OUTPUT))
                {
                    // Do we need to process out variables ?
                    // We can possibly match vertex shader output with a fragment shader inputs
                }
                else if (id->flags & (1 << SpirvId::IS_PUSH_CONSTANT))
                {
                    process_shader_push_constant(structs, structsNum, ids, id, reflection);
                }
                else if (id->flags & (1 << SpirvId::IS_UNIFORM))
                {
                    process_shader_uniform_buffer(structs, structsNum, ids, id, reflection);
                }
            }
        }
    }

    const char* shader_type_to_str(SpirvReflection::ShaderType type)
    {
#define CASE(code) case code: return #code
        switch (type)
        {
            CASE(SpirvReflection::VERTEX);
            CASE(SpirvReflection::FRAGMENT);
        }
        al_vk_assert(!"Unsupported SpirvReflection::ShaderType value");
        return "";
#undef CASE
    }
}

#undef OPCODE
#undef WORDCOUNT
