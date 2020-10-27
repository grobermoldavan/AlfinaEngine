#ifndef AL_LIMITED_INTEGER_H
#define AL_LIMITED_INTEGER_H

#include <cstdint>
#include <concepts>

#include "constexpr_functions.h"

namespace al
{
    template<std::unsigned_integral IntegerType>
    using LuintErrorHandle = void(*)(IntegerType max, IntegerType given);

    template<std::unsigned_integral IntegerType, IntegerType Max, LuintErrorHandle<IntegerType> ErrorHandle = nullptr>
    class luint
    {
    public:
        luint()
            : storedValue{ }
        { }

        luint(IntegerType value)
        {
            set(value);
        }

        ~luint() = default;

        void set(IntegerType value)
        {
            if (value > Max)
            {
                if constexpr (ErrorHandle) ErrorHandle(Max, value);
                return;
            }

            storedValue = value;
        }

        IntegerType get()
        {
            return storedValue;
        }

        // @TODO : add operator overloads

    private:
        IntegerType storedValue;
    };

    template<uint8_t Max, LuintErrorHandle<uint8_t> ErrorHandle = nullptr>
    class luint8_t : public luint<uint8_t, Max, ErrorHandle> { using luint<uint8_t, Max, ErrorHandle>::luint; };

    template<uint16_t Max, LuintErrorHandle<uint16_t> ErrorHandle = nullptr>
    class luint16_t : public luint<uint16_t, Max, ErrorHandle> { using luint<uint16_t, Max, ErrorHandle>::luint; };

    template<uint32_t Max, LuintErrorHandle<uint32_t> ErrorHandle = nullptr>
    class luint32_t : public luint<uint32_t, Max, ErrorHandle> { using luint<uint32_t, Max, ErrorHandle>::luint; };

    template<uint64_t Max, LuintErrorHandle<uint64_t> ErrorHandle = nullptr>
    class luint64_t : public luint<uint64_t, Max, ErrorHandle> { using luint<uint64_t, Max, ErrorHandle>::luint; };

    template<std::unsigned_integral IntegerType, IntegerType MaxBits>
    constexpr IntegerType MaxValueForBits = al::pow<IntegerType>(2, MaxBits) - 1;
}

#endif