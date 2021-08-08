#ifndef AL_HASH_MAP_H
#define AL_HASH_MAP_H

/*
    Simple hash map class.
    Workflow is as follows:
        HashMap<UserKey, UserValue> map;
        hash_map_construct(&map, &allocatorBindings, optionalDefaultCapacity);
        hash_map_add(&map, &userKey, &userValue);
        // add a bunch of values
        for (auto it = create_iterator(&map); !is_finished(&it); advance(&it))
        {
            HashMap<UserKey, UserValue>::Entry* entry = get(it);
            // do stuff
        }
        UserValue* value = hash_map_get(&map, &key);
        hash_map_destroy(&map);

    Can be used directly with string literals (see "Specializations for c-style strings" section):
        hash_map_add(&map, "string key", &value);
        hash_map_add(&map, "string key", "string value");
        hash_map_add(&map, &key, "string value");
*/

#include <cstring>
#include <type_traits>

#include "engine/types.h"
#include "engine/memory/memory.h"

namespace al
{
    template<typename T> concept cstring = std::is_same_v<T, char*> || std::is_same_v<T, const char*>;

    using Hash = u64;

    template<typename Key, typename Value>
    struct HashMap
    {
        static constexpr uSize FREE_ENTRY = 0;
        static constexpr uSize FIRST_VALID_HASH = 1;
        struct Entry
        {
            Hash hash;
            Key key;
            Value value;
        };
        AllocatorBindings allocator;
        Entry* entries;
        uSize capacity;
        uSize size;
    };

    template<typename Key, typename Value>
    struct HashMapIterator
    {
        HashMap<Key, Value>* storage;
        uSize index;
    };

    //
    // This function is used for comparing hash map keys. Can be overriden by user.
    //
    template<typename T>
    bool hash_key_compare(const T* first, const T* second)
    {
        return std::memcmp(first, second, sizeof(T)) == 0;
    }

    template<cstring T>
    bool hash_key_compare(const T* first, const T* second)
    {
        return std::strcmp(*first, *second) == 0;
    }

    //
    // Function for combining two hashes together.
    // Based on boost::hash_combine : https://www.boost.org/doc/libs/1_55_0/doc/html/hash/reference.html#boost.hash_combine
    //
    Hash hash_combine(Hash base, Hash addition)
    {
        return base ^ (addition + 0x9e3779b9 + (base << 6) + (base >> 2));
    }

    //
    // Function for computing hash of a given value.
    // This particular implementation just takes an arbitrary data and hashes it simply
    // using bytes of it's memory footprint.
    // User can make specializations of this function if it's required.
    //
    template<typename T>
    Hash hash_compute(const T* value)
    {
        Hash result = 0;
        uSize numBytes = sizeof(T);
        const Hash* hashPtr = reinterpret_cast<const Hash*>(value);
        while (numBytes >= sizeof(Hash))
        {
            result = hash_combine(result, *hashPtr);
            hashPtr += 1;
            numBytes -= sizeof(Hash);
        }
        const u8* bytePtr = reinterpret_cast<const u8*>(hashPtr);
        while (numBytes > 0)
        {
            result = hash_combine(result, Hash(*bytePtr));
            bytePtr += 1;
            numBytes -= 1;
        }
        return result;
    }

    template<typename Key, typename Value>
    void hash_map_construct(HashMap<Key, Value>* map, AllocatorBindings* allocator, uSize capacity = 128)
    {
        using Entry = HashMap<Key, Value>::Entry;
        map->allocator = *allocator;
        map->entries = allocate<Entry>(&map->allocator, capacity);
        map->capacity = capacity;
        map->size = 0;
        std::memset(map->entries, 0, sizeof(Entry) * capacity);
    }

    template<typename Key, typename Value>
    void hash_map_destroy(HashMap<Key, Value>* map)
    {
        using Entry = HashMap<Key, Value>::Entry;
        deallocate<Entry>(&map->allocator, map->entries, map->capacity);
    }

    //
    // This function is used when we got a hash collision and need to find a free spot for a value
    //
    uSize hash_map_probe_index(uSize currentIndex, uSize capacity)
    {
        return (currentIndex + 1) % capacity;
    }

    template<typename Key, typename Value>
    void hash_map_expand(HashMap<Key, Value>* map)
    {
        using Entry = HashMap<Key, Value>::Entry;
        HashMap<Key, Value> newMap;
        hash_map_construct(&newMap, &map->allocator, map->capacity * 2);
        for (auto it = create_iterator(map); !is_finished(&it); advance(&it))
        {
            hash_map_add(&newMap, &get(it)->key, &get(it)->value);
        }
        hash_map_destroy(map);
        *map = newMap;
    }

    template<typename Key, typename Value>
    void hash_map_add(HashMap<Key, Value>* map, const Key* key, const Value* value)
    {
        using Entry = HashMap<Key, Value>::Entry;
        if ((f64(map->size) / f64(map->capacity)) > 0.7)
        {
            hash_map_expand(map);
        }
        Hash hash = hash_compute(key);
        if (hash < HashMap<Key, Value>::FIRST_VALID_HASH) hash += HashMap<Key, Value>::FIRST_VALID_HASH;
        uSize index = hash % map->capacity;
        while (true)
        {
            Entry* entry = &map->entries[index];
            if (entry->hash < HashMap<Key, Value>::FIRST_VALID_HASH)
            {
                entry->hash = hash;
                entry->key = *key;
                entry->value = *value;
                map->size += 1;
                break;
            }
            else
            {
                index = hash_map_probe_index(index, map->capacity);
            }
        }
    }

    template<typename Key, typename Value>
    HashMap<Key, Value>::Entry* hash_map_find_entry(HashMap<Key, Value>* map, const Key* key)
    {
        //
        // Probing through the map utils we find correct of empty entry.
        // We always need at least one empty entry in the table for the hash_map_find_entry function to work.
        //
        using Entry = HashMap<Key, Value>::Entry;
        Hash hash = hash_compute(key);
        if (hash < HashMap<Key, Value>::FIRST_VALID_HASH) hash += HashMap<Key, Value>::FIRST_VALID_HASH;
        uSize index = hash % map->capacity;
        Entry* entry = &map->entries[index];
        while (entry->hash >= HashMap<Key, Value>::FIRST_VALID_HASH)
        {
            if (entry->hash == hash && hash_key_compare(&entry->key, key))
            {
                return entry;
            }
            else
            {
                index = hash_map_probe_index(index, map->capacity);
                entry = &map->entries[index];
            }
        }
        return nullptr;
    }

    template<typename Key, typename Value>
    Value* hash_map_remove(HashMap<Key, Value>* map, const Key* key)
    {
        using Entry = HashMap<Key, Value>::Entry;
        Entry* entry = hash_map_find_entry(map, key);
        if (entry)
        {
            entry->hash = HashMap<Key, Value>::FREE_ENTRY;
            map->size -= 1;
        }
        return entry ? &entry->value : nullptr;
    }

    template<typename Key, typename Value>
    Value* hash_map_get(HashMap<Key, Value>* map, Key* key)
    {
        using Entry = HashMap<Key, Value>::Entry;
        Entry* entry = hash_map_find_entry(map, key);
        return entry ? &entry->value : nullptr;
    }

    //
    // Iterator
    //

    template<typename Key, typename Value> HashMapIterator<Key, Value> create_iterator(HashMap<Key, Value>* storage)
    {
        uSize index = 0;
        do
        {
            index += 1;
        } while (storage->entries[index].hash == HashMap<Key, Value>::FREE_ENTRY);
        return { storage, index };
    }
    
    template<typename Key, typename Value>
    void advance(HashMapIterator<Key, Value>* iterator)
    {
        do
        {
            iterator->index += 1;
        } while (iterator->storage->entries[iterator->index].hash == HashMap<Key, Value>::FREE_ENTRY);
    }

    template<typename Key, typename Value>
    bool is_finished(HashMapIterator<Key, Value>* iterator)
    {
        return iterator->index >= iterator->storage->capacity;
    }

    template<typename Key, typename Value>
    HashMap<Key, Value>::Entry* get(HashMapIterator<Key, Value> iterator)
    {
        return &iterator.storage->entries[iterator.index];
    }

    //
    // Specializations for c-style strings
    //

    template<>
    Hash hash_compute<const char*>(const char* const* value)
    {
        Hash result = 0;
        uSize numBytes = std::strlen(*value);
        const Hash* hashPtr = reinterpret_cast<const Hash*>(*value);
        while (numBytes >= sizeof(Hash))
        {
            result = hash_combine(result, *hashPtr);
            hashPtr += 1;
            numBytes -= sizeof(Hash);
        }
        const u8* bytePtr = reinterpret_cast<const u8*>(hashPtr);
        while (numBytes > 0)
        {
            result = hash_combine(result, hash_compute(bytePtr));
            bytePtr += 1;
            numBytes -= 1;
        }
        return result;
    }

    template<>
    Hash hash_compute<char*>(char* const* value)
    {
        return hash_compute(const_cast<const char* const*>(value));
    }

    template<cstring Key, typename Value>
    void hash_map_add(HashMap<Key, Value>* map, Key key, const Value* value)
    {
        hash_map_add(map, &key, value);
    }

    template<cstring Key, cstring Value>
    void hash_map_add(HashMap<Key, Value>* map, Key key, Value value)
    {
        hash_map_add(map, &key, &value);
    }

    template<typename Key, cstring Value>
    void hash_map_add(HashMap<Key, Value>* map, const Key* key, Value value)
    {
        hash_map_add(map, key, &value);
    }

    template<cstring Key, typename Value>
    Value hash_map_remove(HashMap<Key, Value>* map, Key key)
    {
        return *hash_map_remove(map, &key);
    }

    template<cstring Key, typename Value>
    Value hash_map_get(HashMap<Key, Value>* map, Key key)
    {
        return *hash_map_get(map, &key);
    }
}

#endif
