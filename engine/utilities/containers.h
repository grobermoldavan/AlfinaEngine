#ifndef AL_ARRAY_VIEW_H
#define AL_ARRAY_VIEW_H

#include "engine/types.h"
#include "engine/memory/memory.h"

#define al_iterator(itName, container) auto itName = create_iterator(&container); !is_finished(&itName); advance(&itName)

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

    template<typename T, uSize Size>
    struct StaticPointerWithSize
    {
        static constexpr uSize size = Size;
        T ptr[Size];
        T& operator [] (uSize index);
    };

    template<typename T, uSize Size>
    T& StaticPointerWithSize<T, Size>::operator [] (uSize index)
    {
        return ptr[index];
    }

    template<typename T, uSize Size>
    struct StaticPointerWithSizeIterator
    {
        StaticPointerWithSize<T, Size>* storage;
        uSize index;
    };

    template<typename T, uSize Size> StaticPointerWithSizeIterator<T, Size> create_iterator(StaticPointerWithSize<T, Size>* storage) { return { storage, 0, }; }
    template<typename T, uSize Size> void advance(StaticPointerWithSizeIterator<T, Size>* iterator) { iterator->index += 1; }
    template<typename T, uSize Size> bool is_finished(StaticPointerWithSizeIterator<T, Size>* iterator) { return iterator->index >= iterator->storage->size; }
    template<typename T, uSize Size> T* get(StaticPointerWithSizeIterator<T, Size> iterator) { return iterator.storage->ptr + iterator.index; }
    template<typename T, uSize Size> uSize to_index(StaticPointerWithSizeIterator<T, Size> iterator) { return iterator.index; }

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

    template<typename T, uSize Size>
    struct DefaultArrayIterator
    {
        T* storage;
        uSize index;
    };

    template<typename T, uSize Size> DefaultArrayIterator<T, Size> create_iterator(T (*storage)[Size]) { return { *storage, 0, }; }
    template<typename T, uSize Size> void advance(DefaultArrayIterator<T, Size>* iterator) { iterator->index += 1; }
    template<typename T, uSize Size> bool is_finished(DefaultArrayIterator<T, Size>* iterator) { return iterator->index >= Size; }
    template<typename T, uSize Size> T* get(DefaultArrayIterator<T, Size> iterator) { return &iterator.storage[iterator.index]; }
    template<typename T, uSize Size> uSize to_index(DefaultArrayIterator<T, Size> iterator) { return iterator.index; }
}

#endif
