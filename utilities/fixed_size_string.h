#ifndef AL_FIXED_SIZE_STRING_H
#define AL_FIXED_SIZE_STRING_H

#include <cstddef>
#include <cstring>

namespace al
{
    template<std::size_t Size>
    class FixedSizeString
    {
    public:
        FixedSizeString() noexcept
            : buffer{ 0 }
        { }

        FixedSizeString(const char* cstr) noexcept
            : buffer{ 0 }
        {
            set(cstr);
        }

        FixedSizeString(const FixedSizeString& other) noexcept
            : buffer{ 0 }
        {
            std::memcpy(buffer, other.buffer, Size);
        }

        bool set(const char* cstr) noexcept
        {
            if (!cstr)
            {
                return false;
            }
            std::size_t length = std::strlen(cstr);
            if (length > Size - 1)
            {
                // String is too long
                return false;
            }
            clear();
            std::memcpy(buffer, cstr, length);
            return true;
        }

        inline void clear() noexcept
        {
            std::memset(buffer, 0, Size);
        }

        bool append(const char* cstr) noexcept
        {
            if (!cstr)
            {
                return false;
            }
            std::size_t currentLength = std::strlen(buffer);
            std::size_t appendLength = std::strlen(cstr);
            if (currentLength + appendLength > Size - 1)
            {
                // Not enough space
                return false;
            }
            std::memcpy(buffer + currentLength, cstr, appendLength);
            return true;
        }

        bool is_equal(const FixedSizeString& other)
        {
            return std::strncmp(buffer, other.buffer, Size) == 0;
        }

        operator char* () noexcept
        {
            return buffer;
        }

        FixedSizeString<Size>& operator = (const char* cstr) noexcept
        {
            set(cstr);
            return *this;
        }

        FixedSizeString<Size>& operator += (const char* cstr) noexcept
        {
            append(cstr);
            return *this;
        }

        bool operator == (const FixedSizeString& other)
        {
            return is_equal(other);
        }

    private:
        char buffer[Size];
    };
}

#endif
