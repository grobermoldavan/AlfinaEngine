#ifndef AL_CONTAINERS_H
#define AL_CONTAINERS_H

#include <type_traits>

#include "engine/types.h"
#include "engine/memory/memory.h"

#define al_iterator(itName, container) auto itName = ::al::create_iterator(&(container)); !::al::is_finished(&itName); ::al::advance(&itName)

namespace al
{
    //
    // Data block storage
    //

    // template<uSize DataBlockSize>
    // constexpr uSize __get_data_block_storage_ledger_size()
    // {
    //     static_assert((DataBlockSize & (DataBlockSize - 1)) == 0, "DataBlockSize must be a power of two");
    //     static_assert(DataBlockSize >= 8, "DataBlockSize must above 8");
    //     return DataBlockSize / 8;
    // }

    template<typename T, uSize DataBlockSize>
    struct DataBlockStorage
    {
        struct DataBlock
        {
            DataBlock* next;
            // u8 ledger[__get_data_block_storage_ledger_size<DataBlockSize>()];
            T data[DataBlockSize];
        };
        AllocatorBindings bindings;
        DataBlock* firstBlock;
        uSize capacity;
        uSize size;
        T& operator [] (uSize index);
    };

    template<typename T, uSize DataBlockSize>
    T& DataBlockStorage<T, DataBlockSize>::operator [] (uSize index)
    {
        uSize blockIndex = index / DataBlockSize;
        uSize inBlockIndex = index % DataBlockSize;
        DataBlock* block = firstBlock;
        for (uSize it = 0; it < blockIndex; it++)
        {
            block = block->next;
        }
        return block->data[inBlockIndex];
    }

    template<typename T, uSize DataBlockSize>
    void data_block_storage_construct(DataBlockStorage<T, DataBlockSize>* storage, AllocatorBindings* bindings)
    {
        using DataBlock = DataBlockStorage<T, DataBlockSize>::DataBlock;
        storage->bindings = *bindings;
        storage->firstBlock = allocate<DataBlock>(&storage->bindings);
        storage->capacity = DataBlockSize;
        storage->size = 0;
        std::memset(storage->firstBlock, 0, sizeof(DataBlock));
    }

    template<typename T, uSize DataBlockSize>
    T* data_block_storage_add(DataBlockStorage<T, DataBlockSize>* storage)
    {
        using DataBlock = DataBlockStorage<T, DataBlockSize>::DataBlock;
        if (storage->size == storage->capacity)
        {
            DataBlock* lastBlock = storage->firstBlock;
            while (lastBlock->next) lastBlock = lastBlock->next;
            lastBlock->next = allocate<DataBlock>(&storage->bindings);
            std::memset(lastBlock->next, 0, sizeof(DataBlock));
            storage->capacity += DataBlockSize;
        }
        return &(*storage)[storage->size++];
    }

    template<typename T, uSize DataBlockSize>
    bool data_block_storage_remove(DataBlockStorage<T, DataBlockSize>* storage, uSize index)
    {
        using DataBlock = DataBlockStorage<T, DataBlockSize>::DataBlock;
        if (index >= storage->size)
        {
            return false;
        }
        T* last = &(*storage)[storage->size - 1];
        if (index < (storage->size - 1))
        {
            (*storage)[index] = *last;
        }
        std::memset(last, 0, sizeof(T));
        storage->size -= 1;
        return true;
    }

    template<typename T, uSize DataBlockSize>
    bool data_block_storage_remove(DataBlockStorage<T, DataBlockSize>* storage, T* entry)
    {
        using DataBlock = DataBlockStorage<T, DataBlockSize>::DataBlock;
        uSize index = storage->size;
        DataBlock* block = storage->firstBlock;
        uSize blockIt = 0;
        while (block->next)
        {
            if ((entry - block->data) >= DataBlockSize || entry < block->data)
            {
                block = block->next;
                blockIt++;
                continue;
            }
            index = blockIt * DataBlockSize + (entry - block->data);
            break;
        }
        return data_block_storage_remove(storage, index);
    }

    template<typename T, uSize DataBlockSize>
    void data_block_storage_destruct(DataBlockStorage<T, DataBlockSize>* storage)
    {
        using DataBlock = DataBlockStorage<T, DataBlockSize>::DataBlock;
        DataBlock* block = storage->firstBlock;
        while (block)
        {
            DataBlock* blockToDelete = block;
            block = block->next;
            deallocate<DataBlock>(&storage->bindings, blockToDelete);
        }
        std::memset(storage, 0, sizeof(DataBlockStorage<T, DataBlockSize>));
    }

    template<typename T, uSize DataBlockSize>
    struct DataBlockStorageIterator
    {
        DataBlockStorage<T, DataBlockSize>* storage;
        uSize index;
    };

    template<typename T, uSize DataBlockSize>
    DataBlockStorageIterator<T, DataBlockSize> create_iterator(DataBlockStorage<T, DataBlockSize>* storage)
    {
        return
        {
            .storage = storage,
            .index = 0,
        };
    }

    template<typename T, uSize DataBlockSize>
    void advance(DataBlockStorageIterator<T, DataBlockSize>* iterator)
    {
        iterator->index += 1;
    }

    template<typename T, uSize DataBlockSize>
    bool is_finished(DataBlockStorageIterator<T, DataBlockSize>* iterator)
    {
        return iterator->index >= iterator->storage->size;
    }

    template<typename T, uSize DataBlockSize>
    T* get(DataBlockStorageIterator<T, DataBlockSize> iterator)
    {
        return &(*iterator.storage)[iterator.index];
    }

    //
    // Pointer with size
    //

    template<typename T>
    struct PointerWithSize
    {
        T* ptr;
        uSize size;
        T& operator [] (uSize index);
    };

    template<typename T>
    T& PointerWithSize<T>::operator [] (uSize index)
    {
        return ptr[index];
    }

    template<typename T>
    struct PointerWithSizeIterator
    {
        PointerWithSize<T>* storage;
        uSize index;
    };

    template<typename T> PointerWithSizeIterator<T> create_iterator(PointerWithSize<T>* storage) { return { storage, 0, }; }
    template<typename T> void advance(PointerWithSizeIterator<T>* iterator) { iterator->index += 1; }
    template<typename T> bool is_finished(PointerWithSizeIterator<T>* iterator) { return iterator->index >= iterator->storage->size; }
    template<typename T> T* get(PointerWithSizeIterator<T> iterator) { return &(*iterator.storage)[iterator.index]; }
    template<typename T> uSize to_index(PointerWithSizeIterator<T> iterator) { return iterator.index; }

    //
    // Array
    //

    template<typename T>
    struct Array
    {
        AllocatorBindings bindings;
        T* memory;
        uSize size;
        T& operator [] (uSize index);
    };

    template<typename T>
    void array_construct(Array<T>* array, AllocatorBindings* bindings, uSize size)
    {
        array->bindings = *bindings;
        array->size = size;
        array->memory = allocate<T>(bindings, size);
        std::memset(array->memory, 0, size * sizeof(T));
    }

    template<typename T>
    void array_destruct(Array<T>* array)
    {
        deallocate<T>(&array->bindings, array->memory, array->size);
    }

    template<typename T>
    T& Array<T>::operator [] (uSize index)
    {
        return memory[index];
    }

    template<typename T>
    struct ArrayIterator
    {
        Array<T>* storage;
        uSize index;
    };

    template<typename T> ArrayIterator<T> create_iterator(Array<T>* storage) { return { storage, 0, }; }
    template<typename T> void advance(ArrayIterator<T>* iterator) { iterator->index += 1; }
    template<typename T> bool is_finished(ArrayIterator<T>* iterator) { return iterator->index >= iterator->storage->size; }
    template<typename T> T* get(ArrayIterator<T> iterator) { return &(*iterator.storage)[iterator.index]; }
    template<typename T> uSize to_index(ArrayIterator<T> iterator) { return iterator.index; }

    //
    // Dynamic Array
    //

    template<typename T>
    struct DynamicArray
    {
        AllocatorBindings bindings;
        T* memory;
        uSize size;
        uSize capacity;
        T& operator [] (uSize index);
    };

    template<typename T>
    void dynamic_array_construct(DynamicArray<T>* array, AllocatorBindings* bindings, uSize initialCapcity = 4)
    {
        array->bindings = *bindings;
        array->size = 0;
        array->capacity = initialCapcity;
        array->memory = allocate<T>(bindings, initialCapcity);
        std::memset(array->memory, 0, initialCapcity * sizeof(T));
    }

    template<typename T>
    void dynamic_array_free(DynamicArray<T>* array, void (*destructor)(T* obj, void* userData) = nullptr, void* userData = nullptr)
    {
        if (destructor) for (uSize it = 0; it < array->size; it++) destructor(&array->memory[it], userData);
        array->size = 0;
    }

    template<typename T>
    void dynamic_array_destruct(DynamicArray<T>* array, void (*destructor)(T* obj, void* userData) = nullptr, void* userData = nullptr)
    {
        dynamic_array_free(array, destructor, userData);
        deallocate<T>(&array->bindings, array->memory, array->capacity);
        std::memset(array, 0, sizeof(DynamicArray<T>));
    }

    template<typename T>
    T* dynamic_array_add(DynamicArray<T>* array)
    {
        if (array->size == array->capacity)
        {
            const uSize newCapacity = array->capacity * 2;
            T* newMemory = allocate<T>(&array->bindings, newCapacity);
            std::memcpy(newMemory, array->memory, sizeof(T) * array->capacity);
            std::memset(newMemory + array->capacity, 0, (newCapacity - array->capacity) * sizeof(T));
            array->capacity = newCapacity;
        }
        return &array->memory[array->size++];
    }

    template<typename T>
    void dynamic_array_remove(DynamicArray<T>* array, uSize index)
    {
        if (index != (array->size - 1))
        {
            std::memcpy(&(*array)[index], &(*array)[array->size - 1], sizeof(T));
        }
        std::memset((*array)[array->size - 1], 0, sizeof(T));
        array->size -= 1;
    }

    template<typename T>
    T& DynamicArray<T>::operator [] (uSize index)
    {
        return memory[index];
    }

    template<typename T>
    struct DynamicArrayIterator
    {
        DynamicArray<T>* storage;
        uSize index;
    };

    template<typename T> DynamicArrayIterator<T> create_iterator(DynamicArray<T>* storage) { return { storage, 0, }; }
    template<typename T> void advance(DynamicArrayIterator<T>* iterator) { iterator->index += 1; }
    template<typename T> bool is_finished(DynamicArrayIterator<T>* iterator) { return iterator->index >= iterator->storage->size; }
    template<typename T> T* get(DynamicArrayIterator<T> iterator) { return &(*iterator.storage)[iterator.index]; }
    template<typename T> uSize to_index(DynamicArrayIterator<T> iterator) { return iterator.index; }

    template<typename T, uSize Size>
    struct CArrayIterator
    {
        T* storage;
        uSize index;
    };

    template<typename T, uSize Size> CArrayIterator<T, Size> create_iterator(T (*storage)[Size]) { return { *storage, 0, }; }
    template<typename T, uSize Size> void advance(CArrayIterator<T, Size>* iterator) { iterator->index += 1; }
    template<typename T, uSize Size> bool is_finished(CArrayIterator<T, Size>* iterator) { return iterator->index >= Size; }
    template<typename T, uSize Size> T* get(CArrayIterator<T, Size> iterator) { return &iterator.storage[iterator.index]; }
    template<typename T, uSize Size> uSize to_index(CArrayIterator<T, Size> iterator) { return iterator.index; }
}

#endif
