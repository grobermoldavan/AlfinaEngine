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

        bool set(const char* cstr) noexcept
        {
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

        operator char* () noexcept
        {
            return buffer;
        }

        FixedSizeString<Size>& operator = (const char* cstr) noexcept
        {
            set(cstr);
            return *this;
        }
        
    private:
        char buffer[Size];
    };
}

#endif
