#ifndef AL_QUATERNION_H
#define AL_QUATERNION_H

#include <cmath>

#include "vectors.h"
#include "utilities/constexpr_functions.h"

namespace al
{
    class Quaternion
    {
    public:
        struct AxisAngle
        {
            float3 axis;
            float angle;
        };

        Quaternion() noexcept;
        Quaternion(const float3&, float) noexcept;
        Quaternion(const Quaternion&) noexcept;

        void        set_axis_angle(const float3& axis, float angle)         noexcept;
        AxisAngle   get_axis_angle()                                const   noexcept;
        void        set_euler_angles(float3 angles)                         noexcept;
        float3      get_euler_angles()                              const   noexcept;

        Quaternion add(const Quaternion& other) const noexcept;
        Quaternion sub(const Quaternion& other) const noexcept;
        Quaternion mul(const Quaternion& other) const noexcept;
        Quaternion mul(float scalar)            const noexcept;
        Quaternion div(const Quaternion& other) const noexcept;
        Quaternion div(float scalar)            const noexcept;

        float       length_squared()    const   noexcept;
        float       length()            const   noexcept;
        void        normalize()                 noexcept;
        Quaternion  normalized()        const   noexcept;
        void        invert()                    noexcept;
        Quaternion  inverted()          const   noexcept;

    private:
        float3 imaginary;
        float real;
    };

    Quaternion::Quaternion() noexcept
        : imaginary{ 0, 0, 0 }
        , real{ 1 }
    { }

    Quaternion::Quaternion(const float3& _imaginary, float _real) noexcept
        : imaginary{ _imaginary }
        , real{ _real }
    { }

    Quaternion::Quaternion(const Quaternion& other) noexcept
        : imaginary{ other.imaginary }
        , real{ other.real }
    { }

    void Quaternion::set_axis_angle(const float3& axis, float angle) noexcept
    {
        float radians = to_radians(angle);
        real = std::cos(radians / 2.0f);
        imaginary = axis * std::sin(radians / 2.0f);
    }

    Quaternion::AxisAngle Quaternion::get_axis_angle() const noexcept
    {
        float acosAngle = std::acos(real);
        float sinAngle = std::sin(acosAngle);

        if (is_equal(std::abs(sinAngle), 0.0f))
        {
            return 
            {
                { 0, 1, 0 },
                to_degrees(acosAngle * 2.0f)
            };
        }
        else
        {
            return
            {
                imaginary / sinAngle,
                to_degrees(acosAngle * 2.0f)
            };
        }
    }

    void Quaternion::set_euler_angles(float3 angles) noexcept
    {
        float cy = std::cos(to_radians(angles.z) * 0.5f);
        float sy = std::sin(to_radians(angles.z) * 0.5f);
        float cp = std::cos(to_radians(angles.y) * 0.5f);
        float sp = std::sin(to_radians(angles.y) * 0.5f);
        float cr = std::cos(to_radians(angles.x) * 0.5f);
        float sr = std::sin(to_radians(angles.x) * 0.5f);

        imaginary.x = sr * cp * cy - cr * sp * sy;
        imaginary.y = cr * sp * cy + sr * cp * sy;
        imaginary.z = cr * cp * sy - sr * sp * cy;
        real = cr * cp * cy + sr * sp * sy;
    }

    float3 Quaternion::get_euler_angles() const noexcept
    {
        float test = imaginary.x * imaginary.y + imaginary.z * real;
        if (test > 0.5f || is_equal(test, 0.5f))
        {
            return
            {
                0.0f,
                2.0f * to_degrees(std::atan2(imaginary.x, real)),
                90.0f
            };
        }
        else if (test < -0.5f || is_equal(test, -0.5f))
        {
            return 
            {
                0.0f,
                -1.0f * 2.0f * to_degrees(std::atan2(imaginary.x, real)),
                -1.0f * 90.0f
            };
        }

        float3 result;

        float squareX = imaginary.x * imaginary.x;
        float squareY = imaginary.y * imaginary.y;
        float squareZ = imaginary.z * imaginary.z;

        float denom = 1.0f - 2.0f * (squareY + squareZ);
        result.y = to_degrees(std::atan2(2.0f * (imaginary.y * real - imaginary.x * imaginary.z), denom));
        result.z = to_degrees(std::asin(2.0f * test));

        denom = 1.0f - 2.0f * (squareX + squareZ);
        result.x = to_degrees(std::atan2(2.0f * (imaginary.x * real - imaginary.y * imaginary.z), denom));

        return result;
    }

    Quaternion Quaternion::add(const Quaternion& other) const noexcept
    {
        return
        {
            imaginary + other.imaginary,
            real + other.real
        };
    }

    Quaternion Quaternion::sub(const Quaternion& other) const noexcept
    {
        return
        {
            imaginary - other.imaginary,
            real - other.real
        };
    }

    Quaternion Quaternion::mul(const Quaternion& other) const noexcept
    {
        return
        {
            other.imaginary * real + imaginary * other.real + imaginary.cross(other.imaginary),
            real * other.real * imaginary.dot(other.imaginary)
        };
    }

    Quaternion Quaternion::mul(float scalar) const noexcept
    {
        return
        {
            imaginary * scalar,
            real * scalar
        };
    }

    Quaternion Quaternion::div(const Quaternion& other) const noexcept
    {
        return mul(other.inverted());
    }

    Quaternion Quaternion::div(float scalar) const noexcept
    {
        return
        {
            imaginary / scalar,
            real / scalar
        };
    }

    float Quaternion::length_squared() const noexcept
    {
        return real * real + imaginary.dot(imaginary);
    }

    float Quaternion::length() const noexcept
    {
        return std::sqrt(length_squared());
    }

    void Quaternion::normalize() noexcept
    {
        const float len = length();
        imaginary = imaginary / len;
        real = real / len;
    }

    Quaternion Quaternion::normalized() const noexcept
    {
        return div(length());
    }

    void Quaternion::invert() noexcept
    {
        imaginary = imaginary * -1.0f;
    }

    Quaternion Quaternion::inverted() const noexcept
    {
        return
        {
            imaginary * -1.0f,
            real
        };
    }
}

#endif
