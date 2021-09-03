
#include <cstring>

#include "spirv_reflection.h"

#define OPCODE(word) u16(word)
#define WORDCOUNT(word) u16(instruction[0] >> 16)

namespace al
{
    struct SpirvId
    {
        struct Decorations { enum : u32
        {
            Block                   = u32(1) << 0,
            BufferBlock             = u32(1) << 1,
            Offset                  = u32(1) << 2,
            InputAttachmentIndex    = u32(1) << 3,
            Location                = u32(1) << 4,
            Binding                 = u32(1) << 5,
            Set                     = u32(1) << 6,
            BuiltIn                 = u32(1) << 7,
        }; };
        SpirvWord* declarationLocation;
        char* name;
        SpirvWord storageClass;
        u32 decorations;
        u32 location;
        u32 binding;
        u32 set;
        u32 inputAttachmentIndex;
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

    template<typename T>
    static T* srs_allocate(SpirvReflection* reflection, uSize count = 1)
    {
        uSize bytesToAllocate = sizeof(T) * count;
        SpirvReflection::MemoryBuffer* buffer = reflection->memory;
        // 1. Try to find memory buffer with enough free space to store new memory
        do
        {
            if ((buffer->size + bytesToAllocate) <= buffer->capacity) break;
            buffer = buffer->next;
        } while (buffer);
        // 2. If no buffer was found, allocate new one
        if (!buffer)
        {
            buffer = reflection->memory;
            while (buffer->next) buffer = buffer->next;
            buffer->next = allocate<SpirvReflection::MemoryBuffer>(&reflection->allocator);
            al_srs_memset(buffer->next, 0, sizeof(SpirvReflection::MemoryBuffer));
            buffer = buffer->next;
        }
        // 3. Reserve space for new memory
        u8* memory = buffer->memory + buffer->size;
        buffer->size += bytesToAllocate;
        return (T*)memory;
    }

    static SpirvReflection::ShaderType execution_mode_to_shader_type(SpvExecutionModel model)
    {
        switch (model)
        {
            case SpvExecutionModelVertex:   return SpirvReflection::Vertex;
            case SpvExecutionModelFragment: return SpirvReflection::Fragment;
        }
        al_srs_assert(!"Unsupported SpvExecutionModel value");
        return SpirvReflection::ShaderType(0);
    }

    static char* save_string_to_buffer(const char* str, SpirvReflection* reflection)
    {
        uSize length = std::strlen(str);
        char* result = srs_allocate<char>(reflection, length + 1);
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
            case SpvOpTypeFloat:        return SpirvReflection::TypeInfo::Scalar;
            case SpvOpTypeInt:          return SpirvReflection::TypeInfo::Scalar;
            case SpvOpTypeVector:       return SpirvReflection::TypeInfo::Vector;
            case SpvOpTypeMatrix:       return SpirvReflection::TypeInfo::Matrix;
            case SpvOpTypeStruct:       return SpirvReflection::TypeInfo::Structure;
            case SpvOpTypeSampler:      return SpirvReflection::TypeInfo::Sampler;
            case SpvOpTypeSampledImage: return SpirvReflection::TypeInfo::Image;
            case SpvOpTypeImage:        return SpirvReflection::TypeInfo::Image;
            case SpvOpTypeRuntimeArray: return SpirvReflection::TypeInfo::Array;
            case SpvOpTypeArray:        return SpirvReflection::TypeInfo::Array;
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
        u16 spvType = OPCODE(id->declarationLocation[0]);
        resultType.kind = op_type_to_value_kind(id->declarationLocation[0]);
        switch(resultType.kind)
        {
            case SpirvReflection::TypeInfo::Scalar:
            {
                decltype(resultType.info.scalar)* scalar = &resultType.info.scalar;
                switch(spvType)
                {
                    case SpvOpTypeFloat:
                    {
                        scalar->kind = SpirvReflection::TypeInfo::ScalarKind::Float;
                        scalar->isSigned = true;
                        scalar->bitWidth = id->declarationLocation[2];
                    } break;
                    case SpvOpTypeInt:
                    {
                        scalar->kind = SpirvReflection::TypeInfo::ScalarKind::Integer;
                        scalar->isSigned = id->declarationLocation[3] == 1;
                        scalar->bitWidth = id->declarationLocation[2];
                    } break;
                    default: al_srs_assert(!"Unsupported scalar value type (bool?)");
                }
                matchedType = try_match_scalar_type(&resultType, reflection);
            } break;
            case SpirvReflection::TypeInfo::Vector:
            {
                decltype(resultType.info.vector)* vector = &resultType.info.vector;
                vector->componentsCount = id->declarationLocation[3];
                vector->componentType = save_or_get_type_from_reflection(structs, structsCount, ids, &ids[id->declarationLocation[2]], reflection);
                al_srs_assert(vector->componentType->kind == SpirvReflection::TypeInfo::Scalar);
                matchedType = try_match_vector_type(&resultType, reflection);
            } break;
            case SpirvReflection::TypeInfo::Matrix:
            {
                decltype(resultType.info.matrix)* matrix = &resultType.info.matrix;
                matrix->columnsCount = id->declarationLocation[3];
                matrix->columnType = save_or_get_type_from_reflection(structs, structsCount, ids, &ids[id->declarationLocation[2]], reflection);
                al_srs_assert(matrix->columnType->kind == SpirvReflection::TypeInfo::Vector);
                matchedType = try_match_matrix_type(&resultType, reflection);
            } break;
            case SpirvReflection::TypeInfo::Structure:
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
                    structure->members = srs_allocate<SpirvReflection::TypeInfo*>(reflection, structure->membersCount);
                    structure->memberNames = srs_allocate<const char*>(reflection, structure->membersCount);
                    for (uSize it = 0; it < structure->membersCount; it++)
                    {
                        structure->members[it] = save_or_get_type_from_reflection(structs, structsCount, ids, runtimeStruct->members[it].id, reflection);
                        structure->memberNames[it] = save_string_to_buffer(runtimeStruct->members[it].name, reflection);
                    }
                }
            } break;
            case SpirvReflection::TypeInfo::Sampler:
            {
                // Nothing (?)
            } break;
            case SpirvReflection::TypeInfo::Image:
            {
                al_srs_assert((spvType == SpvOpTypeSampledImage || spvType == SpvOpTypeImage) && "Unknown image type");
                decltype(resultType.info.image)* image = &resultType.info.image;
                SpirvId* imageId = id;
                if (spvType == SpvOpTypeSampledImage)
                {
                    image->kind = SpirvReflection::TypeInfo::ImageKind::SampledImage;
                    // OpTypeSampledImage just references OpTypeImage:
                    // https://www.khronos.org/registry/SPIR-V/specs/unified1/SPIRV.html#OpTypeSampledImage
                    imageId = &ids[id->declarationLocation[2]];
                }
                else if (spvType == SpvOpTypeImage)
                {
                    image->kind = SpirvReflection::TypeInfo::ImageKind::Image;
                }
                else al_srs_assert(!"Unknown image type");
                al_srs_assert(OPCODE(imageId->declarationLocation[0]) == SpvOpTypeImage);
                SpirvId* sampledTypeId = &ids[imageId->declarationLocation[2]];
                const SpvDim imageDim = SpvDim(imageId->declarationLocation[3]);
                const SpirvWord imageDepth = imageId->declarationLocation[4];
                const SpirvWord isArrayed = imageId->declarationLocation[5];
                const SpirvWord isMultisampled = imageId->declarationLocation[6];
                const SpirvWord imageSampledInfo = imageId->declarationLocation[7];
                const SpvImageFormat imageFormat = SpvImageFormat(imageId->declarationLocation[8]);
                image->sampledType = save_or_get_type_from_reflection(structs, structsCount, ids, sampledTypeId, reflection);
                image->dim = [](const SpvDim dim)
                {
                    if (dim == SpvDim1D) return SpirvReflection::TypeInfo::ImageDim::_1D;
                    else if (dim == SpvDim2D) return SpirvReflection::TypeInfo::ImageDim::_2D;
                    else if (dim == SpvDim3D) return SpirvReflection::TypeInfo::ImageDim::_3D;
                    else if (dim == SpvDimCube) return SpirvReflection::TypeInfo::ImageDim::Cube;
                    else if (dim == SpvDimRect) return SpirvReflection::TypeInfo::ImageDim::Rect;
                    else if (dim == SpvDimBuffer) return SpirvReflection::TypeInfo::ImageDim::Buffer;
                    else if (dim == SpvDimSubpassData) return SpirvReflection::TypeInfo::ImageDim::SubpassData;
                    else al_srs_assert(!"Unknown SpvDim value");
                    return SpirvReflection::TypeInfo::ImageDim(0);
                }(imageDim);
                image->depthParameters = [](const SpirvWord depth)
                {
                    if (depth == 0) return SpirvReflection::TypeInfo::ImageDepthParameters::NotDepthImage;
                    else if (depth == 1) return SpirvReflection::TypeInfo::ImageDepthParameters::DepthImage;
                    else if (depth == 2) return SpirvReflection::TypeInfo::ImageDepthParameters::Unknown;
                    else al_srs_assert(!"Unknown OpTypeImage depth value");
                    return SpirvReflection::TypeInfo::ImageDepthParameters(0);
                }(imageDepth);
                image->isArrayed = isArrayed != 0;
                image->isMultisampled = isMultisampled != 0;
                image->sampleParameters = [](const SpirvWord sampledInfo)
                {
                    if (sampledInfo == 0) return SpirvReflection::TypeInfo::ImageSampleParameters::Unknown;
                    else if (sampledInfo == 1) return SpirvReflection::TypeInfo::ImageSampleParameters::UsedWithSampler;
                    else if (sampledInfo == 2) return SpirvReflection::TypeInfo::ImageSampleParameters::NotUsedWithSampler;
                    else al_srs_assert(!"Unknown OpTypeImage sample value");
                    return SpirvReflection::TypeInfo::ImageSampleParameters(0);
                }(imageSampledInfo);
            } break;
            case SpirvReflection::TypeInfo::Array:
            {
                decltype(resultType.info.array)* array = &resultType.info.array;
                switch(spvType)
                {
                    case SpvOpTypeRuntimeArray:
                    {
                        array->size = 0;
                        array->kind = SpirvReflection::TypeInfo::ArrayKind::RuntimeArray;
                    } break;
                    case SpvOpTypeArray:
                    {
                        SpirvId* arrayLength = &ids[id->declarationLocation[3]];
                        SpirvId* arrayLengthType = &ids[arrayLength->declarationLocation[1]];
                        al_srs_assert(OPCODE(arrayLengthType->declarationLocation[0]) == SpvOpTypeInt);
                        SpirvWord arrayLengthTypeBitWidth = arrayLengthType->declarationLocation[2];
                        switch (arrayLengthTypeBitWidth)
                        {
                            case 32: array->size = *((u32*)&arrayLength->declarationLocation[3]); break;
                            default: al_srs_assert(!"Unsupported array size constat bit width");
                        };
                        array->kind = SpirvReflection::TypeInfo::ArrayKind::Array;
                    } break;
                    default: al_srs_assert(!"Unknown array type");
                }
                array->entryType = save_or_get_type_from_reflection(structs, structsCount, ids, &ids[id->declarationLocation[2]], reflection);
                matchedType = try_match_array_type(&resultType, reflection);
            } break;
            default: al_srs_assert(!"Unsupported SpirvReflection::TypeInfo value");
        }
        if (!matchedType)
        {
            matchedType = srs_allocate<SpirvReflection::TypeInfo>(reflection);
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
        if (ioDeclarationId->decorations & SpirvId::Decorations::BuiltIn || id->decorations & SpirvId::Decorations::BuiltIn)
        {
            io->flags |= SpirvReflection::ShaderIO::IsBuiltIn;
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
        reflection->pushConstant = srs_allocate<SpirvReflection::PushConstant>(reflection);
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

    static void process_shader_uniform(SpirvStruct* structs, uSize structsCount, SpirvId* ids, SpirvId* uniformIdOpVariable, SpirvReflection* reflection)
    {
        // https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#interfaces-resources-descset
        // https://github.com/KhronosGroup/Vulkan-Guide/blob/master/chapters/mapping_data_to_shaders.md
        SpirvReflection::Uniform* uniform = &reflection->uniforms[reflection->uniformCount++];
        //
        // Start from variable declaration
        //
        SpirvWord* opVariable = uniformIdOpVariable->declarationLocation;
        al_srs_assert(OPCODE(opVariable[0]) == SpvOpVariable);
        al_srs_assert(uniformIdOpVariable->name);
        al_srs_assert(uniformIdOpVariable->decorations & SpirvId::Decorations::Set);
        al_srs_assert(uniformIdOpVariable->decorations & SpirvId::Decorations::Binding);
        uniform->name = save_string_to_buffer(uniformIdOpVariable->name, reflection);
        uniform->set = uniformIdOpVariable->set;
        uniform->binding = uniformIdOpVariable->binding;
        if (uniformIdOpVariable->decorations & SpirvId::Decorations::InputAttachmentIndex)
        {
            // @TODO : Check if this is correct
            uniform->inputAttachmentIndex = uniformIdOpVariable->inputAttachmentIndex;
        }
        //
        // Go to OpTypePointer describing this variable
        //
        SpirvWord* opTypePointer = ids[opVariable[1]].declarationLocation;
        al_srs_assert(OPCODE(opTypePointer[0]) == SpvOpTypePointer);
        SpirvId* uniformIdOpType = &ids[opTypePointer[3]];
        uniform->typeInfo = save_or_get_type_from_reflection(structs, structsCount, ids, &ids[uniformIdOpType->declarationLocation[1]], reflection);
        u16 opType = OPCODE(uniformIdOpType->declarationLocation[0]);
        SpvStorageClass storageClass = SpvStorageClass(opTypePointer[2]);
        if (opType == SpvOpTypeArray)
        {
            uniformIdOpType = &ids[uniformIdOpType->declarationLocation[2]]; // go to array entry type id
            opType = OPCODE(uniformIdOpType->declarationLocation[0]); // get array entry OpType
        }
        if (storageClass == SpvStorageClassUniform)
        {
            if (opType == SpvOpTypeStruct)
            {
                const bool isBlock = uniformIdOpType->decorations & SpirvId::Decorations::Block;
                const bool isBufferBlock = uniformIdOpType->decorations & SpirvId::Decorations::BufferBlock;
                al_srs_assert((isBlock && !isBufferBlock) || (isBufferBlock && !isBlock));
                if      (isBlock)       uniform->type = SpirvReflection::Uniform::UniformBuffer;
                else if (isBufferBlock) uniform->type = SpirvReflection::Uniform::StorageBuffer;
            }
            else al_srs_assert(!"Uniform with StorageClassUniform has unsupported OpType");
        }
        else if (storageClass == SpvStorageClassUniformConstant)
        {
            if (opType == SpvOpTypeImage)
            {
                const SpirvWord sampled = uniformIdOpType->declarationLocation[7];
                const SpvDim dim = SpvDim(uniformIdOpType->declarationLocation[3]);
                if (sampled == 1)
                {
                    if      (dim == SpvDimBuffer)       uniform->type = SpirvReflection::Uniform::UniformTexelBuffer;
                    else if (dim == SpvDimSubpassData)  al_srs_assert(!"Unsupported uniform type");
                    else                                uniform->type = SpirvReflection::Uniform::SampledImage;
                }
                else if (sampled == 2)
                {
                    if      (dim == SpvDimBuffer)       uniform->type = SpirvReflection::Uniform::StorageTexelBuffer;
                    else if (dim == SpvDimSubpassData)  uniform->type = SpirvReflection::Uniform::InputAttachment;
                    else                                uniform->type = SpirvReflection::Uniform::StorageImage;
                }
                else al_srs_assert(!"Unsupported \"Sampled\" parameter for OpTypeImage");
            }
            else if (opType == SpvOpTypeSampler)                    uniform->type = SpirvReflection::Uniform::Sampler;
            else if (opType == SpvOpTypeSampledImage)               uniform->type = SpirvReflection::Uniform::CombinedImageSampler;
            else if (opType == SpvOpTypeAccelerationStructureKHR)   uniform->type = SpirvReflection::Uniform::AccelerationStructure;
            else al_srs_assert(!"Uniform with StorageClassUniformConstant has unsupported OpType");
        }
        else if (storageClass == SpvStorageClassStorageBuffer)
        {
            if (opType == SpvOpTypeStruct) uniform->type = SpirvReflection::Uniform::StorageBuffer;
            else al_srs_assert(!"Uniform with StorageClassStorageBuffer has unsupported OpType");
        }
        else al_srs_assert(!"Unsupported uniform SpvStorageClass");
    }

    void spirv_reflection_construct(SpirvReflection* reflection, SpirvReflectionCreateInfo* createInfo)
    {
        al_srs_memset(reflection, 0, sizeof(SpirvReflection));
        reflection->allocator = *createInfo->persistentAllocator;
        reflection->memory = allocate<SpirvReflection::MemoryBuffer>(&reflection->allocator);
        al_srs_memset(reflection->memory, 0, sizeof(SpirvReflection::MemoryBuffer));
        SpirvWord* bytecode = createInfo->bytecode;
        uSize wordCount = createInfo->wordCount;
        //
        // Basic header validation + retrieving number of unique ids (bound) used in bytecode
        //
        al_srs_assert(bytecode[0] == SpvMagicNumber);
        uSize bound = bytecode[3];
        SpirvWord* instruction = nullptr;
        //
        // Allocate SpirvId array which will hols all neccessary information about shaders identifiers
        //
        SpirvId* ids = allocate<SpirvId>(createInfo->nonPersistentAllocator, bound);
        al_srs_memset(ids, 0, sizeof(SpirvId) * bound);
        defer(deallocate<SpirvId>(createInfo->nonPersistentAllocator, ids, bound));
        //
        // Step 1. Filling ids array + some general shader info
        //
#define SaveDeclorationLocation(index) { SpirvWord id = instruction[index]; al_srs_assert(id < bound); ids[id].declarationLocation = instruction; }
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
                case SpvOpConstant:         al_srs_assert(instructionWordCount >= 3); SaveDeclorationLocation(2); break;
                case SpvOpTypeFloat:        al_srs_assert(instructionWordCount == 3); SaveDeclorationLocation(1); break;
                case SpvOpTypeInt:          al_srs_assert(instructionWordCount == 4); SaveDeclorationLocation(1); break;
                case SpvOpTypeVector:       al_srs_assert(instructionWordCount == 4); SaveDeclorationLocation(1); break;
                case SpvOpTypeMatrix:       al_srs_assert(instructionWordCount == 4); SaveDeclorationLocation(1); break;
                case SpvOpTypeStruct:       al_srs_assert(instructionWordCount >= 2); SaveDeclorationLocation(1); break;
                case SpvOpTypeSampler:      al_srs_assert(instructionWordCount == 2); SaveDeclorationLocation(1); break;
                case SpvOpTypeSampledImage: al_srs_assert(instructionWordCount == 3); SaveDeclorationLocation(1); break;
                case SpvOpTypeImage:        al_srs_assert(instructionWordCount >= 9); SaveDeclorationLocation(1); break;
                case SpvOpTypeRuntimeArray: al_srs_assert(instructionWordCount == 3); SaveDeclorationLocation(1); break;
                case SpvOpTypeArray:        al_srs_assert(instructionWordCount == 4); SaveDeclorationLocation(1); break;
                case SpvOpTypePointer:
                {
                    al_srs_assert(instructionWordCount == 4);
                    SaveDeclorationLocation(1);
                    ids[instruction[1]].storageClass = instruction[2];
                } break;
                case SpvOpVariable:
                {
                    al_srs_assert(instructionWordCount >= 4);
                    SaveDeclorationLocation(2);
                    SpirvWord storageClass = instruction[3];
                    SpirvWord resultId = instruction[2];
                    ids[resultId].storageClass = storageClass;
                    if      (storageClass == SpvStorageClassInput)              inputsCount += 1;
                    else if (storageClass == SpvStorageClassOutput)             outputsCount += 1;
                    else if (storageClass == SpvStorageClassUniform)            uniformsCount += 1;
                    else if (storageClass == SpvStorageClassUniformConstant)    uniformsCount += 1;
                    else if (storageClass == SpvStorageClassStorageBuffer)      uniformsCount += 1;
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
                            ids[id].decorations |= SpirvId::Decorations::BuiltIn;
                        } break;
                    }
                } break;
                case SpvOpDecorate:
                {
                    SpvDecoration decoration = SpvDecoration(instruction[2]);
                    switch (decoration)
                    {
                        case SpvDecorationBlock:
                        {
                            al_srs_assert(instructionWordCount == 3);
                            SpirvWord id = instruction[1];
                            al_srs_assert(id < bound);
                            ids[id].decorations |= SpirvId::Decorations::Block;
                        } break;
                        case SpvDecorationBufferBlock:
                        {
                            al_srs_assert(instructionWordCount == 3);
                            SpirvWord id = instruction[1];
                            al_srs_assert(id < bound);
                            ids[id].decorations |= SpirvId::Decorations::BufferBlock;
                        } break;
                        case SpvDecorationBuiltIn:
                        {
                            al_srs_assert(instructionWordCount == 4);
                            SpirvWord id = instruction[1];
                            al_srs_assert(id < bound);
                            ids[id].decorations |= SpirvId::Decorations::BuiltIn;
                        } break;
                        case SpvDecorationLocation:
                        {
                            al_srs_assert(instructionWordCount == 4);
                            SpirvWord id = instruction[1];
                            al_srs_assert(id < bound);
                            ids[id].location = decltype(ids[id].location)(instruction[3]);
                            ids[id].decorations |= SpirvId::Decorations::Location;
                        } break;
                        case SpvDecorationBinding:
                        {
                            al_srs_assert(instructionWordCount == 4);
                            SpirvWord id = instruction[1];
                            al_srs_assert(id < bound);
                            ids[id].binding = decltype(ids[id].binding)(instruction[3]);
                            ids[id].decorations |= SpirvId::Decorations::Binding;
                        } break;
                        case SpvDecorationDescriptorSet:
                        {
                            al_srs_assert(instructionWordCount == 4);
                            SpirvWord id = instruction[1];
                            al_srs_assert(id < bound);
                            ids[id].set = decltype(ids[id].set)(instruction[3]);
                            ids[id].decorations |= SpirvId::Decorations::Set;
                        } break;
                        case SpvDecorationInputAttachmentIndex:
                        {
                            al_srs_assert(instructionWordCount == 4);
                            SpirvWord id = instruction[1];
                            al_srs_assert(id < bound);
                            ids[id].inputAttachmentIndex = decltype(ids[id].inputAttachmentIndex)(instruction[3]);
                            ids[id].decorations |= SpirvId::Decorations::InputAttachmentIndex;
                        } break;
                    }
                } break;
            }
            instruction += instructionWordCount;
        }
        reflection->shaderInputs = srs_allocate<SpirvReflection::ShaderIO>(reflection, inputsCount);
        reflection->shaderOutputs = srs_allocate<SpirvReflection::ShaderIO>(reflection, outputsCount);
        reflection->uniforms = srs_allocate<SpirvReflection::Uniform>(reflection, uniformsCount);
#undef SaveDeclorationLocation
        //
        // Step 2. Retrieving information about shader structs
        //
        //
        // Step 2.1. Count number of structs and struct members
        //
        
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
        SpirvStruct* structs = allocate<SpirvStruct>(createInfo->nonPersistentAllocator, structsCount);
        SpirvStruct::Member* structMembers = allocate<SpirvStruct::Member>(createInfo->nonPersistentAllocator, structmembersCount);
        defer(deallocate<SpirvStruct>(createInfo->nonPersistentAllocator, structs, structsCount));
        defer(deallocate<SpirvStruct::Member>(createInfo->nonPersistentAllocator, structMembers, structmembersCount));
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
            if (id->declarationLocation)
            {
                int a = 0;
            }
            if (id->declarationLocation && OPCODE(id->declarationLocation[0]) == SpvOpVariable)
            {
                SpvStorageClass storageClass = SpvStorageClass(ids[id->declarationLocation[2]].storageClass);
                if      (storageClass == SpvStorageClassInput)              process_shader_input_variable(structs, structsCount, ids, id, reflection);
                else if (storageClass == SpvStorageClassOutput)             process_shader_output_variable(structs, structsCount, ids, id, reflection);
                else if (storageClass == SpvStorageClassPushConstant)       process_shader_push_constant(structs, structsCount, ids, id, reflection);
                else if (storageClass == SpvStorageClassUniform)            process_shader_uniform(structs, structsCount, ids, id, reflection);
                else if (storageClass == SpvStorageClassUniformConstant)    process_shader_uniform(structs, structsCount, ids, id, reflection);
                else if (storageClass == SpvStorageClassStorageBuffer)      process_shader_uniform(structs, structsCount, ids, id, reflection);
            }
        }
    }

    void spirv_reflection_destroy(SpirvReflection* reflection)
    {
        SpirvReflection::MemoryBuffer* buffer = reflection->memory;
        while (buffer)
        {
            SpirvReflection::MemoryBuffer* next = buffer->next;
            deallocate<SpirvReflection::MemoryBuffer>(&reflection->allocator, buffer);
            buffer = next;
        }
    }

    void spirv_reflection_print_type_info(SpirvReflection::TypeInfo* typeInfo, SpirvReflection* reflection, uSize indentationLevel, const char* structMemberName)
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
            case SpirvReflection::TypeInfo::Scalar:
            {
                static const char* SCALAR_KINDS[] = { "int", "float" };
                al_srs_printf("%s ", typeInfo->info.scalar.isSigned ? "signed" : "unsigned");
                al_srs_printf("%s", SCALAR_KINDS[static_cast<uSize>(typeInfo->info.scalar.kind)]);
                al_srs_printf("%d.\n", typeInfo->info.scalar.bitWidth);
            } break;
            case SpirvReflection::TypeInfo::Vector:
            {
                al_srs_printf("vec%d of type ", typeInfo->info.vector.componentsCount);
                spirv_reflection_print_type_info(typeInfo->info.vector.componentType, reflection, 0);
            } break;
            case SpirvReflection::TypeInfo::Matrix:
            {
                al_srs_printf("mat%d with column type ", typeInfo->info.matrix.columnsCount);
                spirv_reflection_print_type_info(typeInfo->info.matrix.columnType, reflection, 0);
            } break;
            case SpirvReflection::TypeInfo::Structure:
            {
                al_srs_printf("struct %s with following members:\n", typeInfo->info.structure.typeName);
                for (uSize it = 0; it < typeInfo->info.structure.membersCount; it++)
                {
                    spirv_reflection_print_type_info(typeInfo->info.structure.members[it], reflection, indentationLevel + 1, typeInfo->info.structure.memberNames[it]);
                }
            } break;
            case SpirvReflection::TypeInfo::Array:
            {
                if (typeInfo->info.array.size == 0) al_srs_printf("runtime array ");
                else                                al_srs_printf("array of size %zd ", typeInfo->info.array.size);
                al_srs_printf("with following entry type:\n");
                spirv_reflection_print_type_info(typeInfo->info.array.entryType, reflection, indentationLevel + 1);
            } break;
            default: al_srs_assert(!"Unknown SpirvReflection::TypeInfo::Kind");
        }
    }

    uSize spirv_reflection_get_type_info_size(SpirvReflection::TypeInfo* typeInfo)
    {
        switch(typeInfo->kind)
        {
            case SpirvReflection::TypeInfo::Scalar:
            {
                return typeInfo->info.scalar.bitWidth / 8;
            } break;
            case SpirvReflection::TypeInfo::Vector:
            {
                return typeInfo->info.vector.componentsCount * spirv_reflection_get_type_info_size(typeInfo->info.vector.componentType);
            } break;
            case SpirvReflection::TypeInfo::Matrix:
            {
                return typeInfo->info.matrix.columnsCount * spirv_reflection_get_type_info_size(typeInfo->info.matrix.columnType);
            } break;
            case SpirvReflection::TypeInfo::Structure:
            {
                uSize resultSize = 0;
                for (uSize it = 0; it < typeInfo->info.structure.membersCount; it++)
                {
                    resultSize += spirv_reflection_get_type_info_size(typeInfo->info.structure.members[it]);
                }
                return resultSize;
            } break;
            case SpirvReflection::TypeInfo::Array:
            {
                return typeInfo->info.array.size * spirv_reflection_get_type_info_size(typeInfo->info.array.entryType);
            } break;
            default: al_srs_assert(!"Unknown SpirvReflection::TypeInfo::Kind");
        }
        return 0;
    }

    const char* spirv_reflection_shader_type_to_str(SpirvReflection::ShaderType type)
    {
#define CASE(code) case code: return #code
        switch (type)
        {
            CASE(SpirvReflection::Vertex);
            CASE(SpirvReflection::Fragment);
        }
        al_srs_assert(!"Unsupported SpirvReflection::ShaderType value");
        return "";
#undef CASE
    }
}

#undef OPCODE
#undef WORDCOUNT
