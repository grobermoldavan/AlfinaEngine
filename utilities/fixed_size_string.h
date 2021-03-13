#ifndef AL_FIXED_SIZE_STRING_H
#define AL_FIXED_SIZE_STRING_H

#include <cstddef>  // for std::size_t
#include <cstring>  // for std::memcpy, std::memset, std::strlen

namespace al
{
    template<std::size_t Size>
    struct FixedSizeString
    {
        char memory[Size];
    };

    template<std::size_t Size>
    bool construct(FixedSizeString<Size>* str, const char* cstr = "")
    {
        if (!cstr)
        {
            return false;
        }
        return construct(str, cstr, std::strlen(cstr));
    }

    template<std::size_t Size>
    void construct(FixedSizeString<Size>* str, const FixedSizeString<Size>* other)
    {
        std::memcpy(str->memory, other->memory, Size);
    }

    template<std::size_t Size>
    bool construct(FixedSizeString<Size>* str, const char* cstr, std::size_t length) noexcept
    {
        if (!cstr)
        {
            return false;
        }
        if (length > Size - 1)
        {
            // String is too long
            return false;
        }
        clear(str);
        std::memcpy(str->memory, cstr, length);
        return true;
    }

    template<std::size_t Size>
    inline void clear(FixedSizeString<Size>* str) noexcept
    {
        std::memset(str->memory, 0, Size);
    }

    template<std::size_t Size>
    bool append(FixedSizeString<Size>* str, const char* cstr) noexcept
    {
        if (!cstr)
        {
            return false;
        }
        std::size_t currentLength = std::strlen(str->memory);
        std::size_t appendLength = std::strlen(cstr);
        if ((currentLength + appendLength) > Size - 1)
        {
            // Not enough space
            return false;
        }
        std::memcpy(str->memory + currentLength, cstr, appendLength);
        return true;
    }

    template<std::size_t Size>
    bool is_equal(const FixedSizeString<Size>* first, const FixedSizeString<Size>* second)
    {
        return std::strncmp(first->memory, second->memory, Size) == 0;
    }

    template<std::size_t Size>
    const char* cstr(const FixedSizeString<Size>* str)
    {
        return str->memory;
    }
}

#endif
