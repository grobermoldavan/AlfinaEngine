
#include <cstring>

#include "spirv_reflection.h"

#define OPCODE(word) u16(word)

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

    char* save_string_to_buffer(const char* str, char* buffer, uSize* bufferPtr, uSize bufferSize)
    {
        char* result = nullptr;
        uSize strLen = std::strlen(str);
        al_vk_assert(strLen < (bufferSize - *bufferPtr) && "String buffer overflow");
        result = &buffer[*bufferPtr];
        std::memcpy(result, str, strLen);
        result[strLen] = 0;
        *bufferPtr += strLen + 1;
        return result;
    }

    void process_shader_input_variable(ArrayView<SpirvId> ids, SpirvId* inputVariable, SpirvReflection* reflection)
    {
        al_vk_assert(reflection->shaderInputCount < SpirvReflection::MAX_VERTEX_INPUTS);
        SpirvReflection::ShaderInput* input = &reflection->shaderInputs[reflection->shaderInputCount++];
        input->name = save_string_to_buffer(inputVariable->name, reflection->nameBuffer, &reflection->nameBufferCounter, SpirvReflection::NAME_BUFFER_SIZE);
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
            instruction = ids[instruction[3]].declarationLocation;
            switch (OPCODE(instruction[0]))
            {
                case SpvOpTypeFloat:
                {
                    //
                    // Input is a single float
                    //
                    input->sizeBytes = instruction[2];
                } break;
                case SpvOpTypeInt:
                {
                    //
                    // Input is a single int
                    //
                    input->sizeBytes = instruction[2];
                } break;
                case SpvOpTypeVector:
                {
                    //
                    // Input is a vector of floats or ints
                    //
                    SpirvWord componentsNum = instruction[3];
                    SpirvWord singleComponentSize;
                    instruction = ids[instruction[2]].declarationLocation;
                    switch (OPCODE(instruction[0]))
                    {
                        case SpvOpTypeFloat:
                        {
                            singleComponentSize = instruction[2];
                        } break;
                        case SpvOpTypeInt:
                        {
                            singleComponentSize = instruction[2];
                        } break;
                        default:
                        {
                            al_vk_assert(!"Unsupported vector component type for a shader input");
                        } break;
                    }
                    input->sizeBytes = u16(componentsNum * singleComponentSize);
                } break;
                case SpvOpTypeMatrix:
                {
                    SpirvWord columnsNum = instruction[3];
                    instruction = ids[instruction[2]].declarationLocation;
                    SpirvWord rowsNum = instruction[3];
                    SpirvWord singleComponentSize;
                    instruction = ids[instruction[2]].declarationLocation;
                    switch (OPCODE(instruction[0]))
                    {
                        case SpvOpTypeFloat:
                        {
                            singleComponentSize = instruction[2];
                        } break;
                        case SpvOpTypeInt:
                        {
                            singleComponentSize = instruction[2];
                        } break;
                        default:
                        {
                            al_vk_assert(!"Unsupported matrix component type for a shader input");
                        } break;
                    }
                    input->sizeBytes = u16(rowsNum * columnsNum * singleComponentSize);
                } break;
                default:
                {
                    al_vk_assert(!"Unsupported OpType for a shader input");
                } break;
            }
        }
    }

    void construct_spirv_reflect(AllocatorBindings bindings, SpirvWord* bytecode, uSize wordCount, SpirvReflection* result)
    {
        constexpr uSize ID_NAMES_BUFFER_SIZE = 512;
        char idNamesBuffer[ID_NAMES_BUFFER_SIZE] = {};
        uSize idNamesBufferPtr = 0;

        al_vk_assert(bytecode);
        constexpr uSize INSTRUCTION_STREAM_FIRST_WORD = 5;
        std::memset(result, 0, sizeof(SpirvReflection));
        //
        // Header validation and info
        al_vk_assert(bytecode[0] == SpvMagicNumber);
        uSize bound = bytecode[3];
        ArrayView<SpirvId> ids;
        av_construct(&ids, &bindings, bound);
        //
        // Instruction stream processing
        SpirvWord* instruction = bytecode + INSTRUCTION_STREAM_FIRST_WORD;
        while (instruction < (bytecode + wordCount))
        {
            u16 instructionOpCode = OPCODE(instruction[0]);
            u16 instructionWordCount = u16(instruction[0] >> 16);
            switch (instructionOpCode)
            {
                case SpvOpEntryPoint:
                {
                    al_vk_assert(instructionWordCount >= 4);
                    result->shaderType = execution_mode_to_shader_type(SpvExecutionModel(instruction[1]));
                    result->entryPointName = save_string_to_buffer((char*)&instruction[3], result->nameBuffer, &result->nameBufferCounter, SpirvReflection::NAME_BUFFER_SIZE);
                } break;
                case SpvOpName:
                {
                    al_vk_assert(instructionWordCount >= 3);
                    SpirvWord id = instruction[1];
                    ids[id].name = save_string_to_buffer((char*)&instruction[2], idNamesBuffer, &idNamesBufferPtr, ID_NAMES_BUFFER_SIZE);
                } break;
                case SpvOpMemberName:
                {
                    // TODO ?
                } break;
                case SpvOpTypeFloat:
                {
                    al_vk_assert(instructionWordCount == 3);
                    SpirvWord id = instruction[1];
                    al_vk_assert(id < bound);
                    ids[id].declarationLocation = instruction;
                } break;
                case SpvOpTypeInt:
                {
                    al_vk_assert(instructionWordCount == 4);
                    SpirvWord id = instruction[1];
                    al_vk_assert(id < bound);
                    ids[id].declarationLocation = instruction;
                } break;
                case SpvOpTypeVector:
                {
                    al_vk_assert(instructionWordCount == 4);
                    SpirvWord id = instruction[1];
                    al_vk_assert(id < bound);
                    ids[id].declarationLocation = instruction;
                } break;
                case SpvOpTypeMatrix:
                {
                    al_vk_assert(instructionWordCount == 4);
                    SpirvWord id = instruction[1];
                    al_vk_assert(id < bound);
                    ids[id].declarationLocation = instruction;
                } break;
                case SpvOpTypePointer:
                {
                    al_vk_assert(instructionWordCount == 4);
                    SpirvWord id = instruction[1];
                    al_vk_assert(id < bound);
                    ids[id].declarationLocation = instruction;
                    SpvStorageClass storageClass = SpvStorageClass(instruction[2]);
                    if (storageClass == SpvStorageClassInput)
                    {
                        ids[id].flags |= (1 << SpirvId::IS_INPUT);
                    }
                } break;
                case SpvOpVariable:
                {
                    al_vk_assert(instructionWordCount >= 4);
                    SpirvWord id = instruction[2];
                    al_vk_assert(id < bound);
                    ids[id].declarationLocation = instruction;
                    SpvStorageClass storageClass = SpvStorageClass(instruction[3]);
                    switch (storageClass)
                    {
                        case SpvStorageClassInput:
                        {
                            ids[id].flags |= (1 << SpirvId::IS_INPUT);
                        } break;
                        case SpvStorageClassOutput:
                        {
                            ids[id].flags |= (1 << SpirvId::IS_OUTPUT);
                        } break;
                        case SpvStorageClassPushConstant:
                        {
                            ids[id].flags |= (1 << SpirvId::IS_PUSH_CONSTANT);
                        } break;
                        case SpvStorageClassUniform:
                        {
                            ids[id].flags |= (1 << SpirvId::IS_UNIFORM);
                        } break;
                    }
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
                        } break;
                    }
                } break;
            }
            instruction += instructionWordCount;
       }
       for (uSize it = 0; it < ids.count; it++)
       {
            SpirvId* id = &ids[it];
            if (id->declarationLocation && OPCODE(id->declarationLocation[0]) == SpvOpVariable)
            {
                if (id->flags & (1 << SpirvId::IS_INPUT))
                {
                    process_shader_input_variable(ids, id, result);
                }
                else if (id->flags & (1 << SpirvId::IS_OUTPUT))
                {
                    // Do we need to process out variables ?
                    // We can possibly match vertex shader output with a fragment shader inputs
                }
                else if (id->flags & (1 << SpirvId::IS_PUSH_CONSTANT))
                {
                    
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
