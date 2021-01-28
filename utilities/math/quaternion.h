#ifndef AL_QUATERNION_H
#define AL_QUATERNION_H

#include <cmath>
#include <numbers>
#include <limits>

#include "vectors.h"
#include "matrices.h"
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
        Quaternion(const float3& axis, float angle) noexcept;
        Quaternion(const Quaternion& other) noexcept;

        void        set_axis_angle(const float3& axis, float angle)         noexcept;
        AxisAngle   get_axis_angle()                                const   noexcept;
        void        set_euler_angles(const float3& angles)                  noexcept;
        float3      get_euler_angles()                              const   noexcept;
        void        set_rotation_mat(const float4x4& mat)                   noexcept;
        float4x4    get_rotation_mat()                              const   noexcept;
        void        set_mat(const float4x4& mat)                            noexcept;

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
        Quaternion  conjugate()         const   noexcept;

        friend std::ostream& operator << (std::ostream& os, const Quaternion& quat) noexcept;

    public:
        union
        {
            struct
            {
                float real;
                float3 imaginary;
            };
            struct
            {
                float w, x, y, z;
            };
            float elements[4];
        };
    };

    Quaternion::Quaternion() noexcept
        : imaginary{ 0, 0, 0 }
        , real{ 1 }
    { }

    Quaternion::Quaternion(const float3& axis, float angle) noexcept
        : imaginary{ }
        , real{ }
    {
        set_axis_angle(axis, angle);
    }

    Quaternion::Quaternion(const Quaternion& other) noexcept
        : imaginary{ other.imaginary }
        , real{ other.real }
    { }

    void Quaternion::set_axis_angle(const float3& axis, float angle) noexcept
    {
        // TODO : fix
    }

    Quaternion::AxisAngle Quaternion::get_axis_angle() const noexcept
    {
        // TODO : fix
        return {};
    }

    void Quaternion::set_euler_angles(const float3& angles) noexcept
    {
        // TODO : fix
    }

    float3 Quaternion::get_euler_angles() const noexcept
    {
        // TODO : fix
        return {};
    }

    void Quaternion::set_rotation_mat(const float4x4& mat) noexcept
    {
        // TODO : fix
    }

    float4x4 Quaternion::get_rotation_mat() const noexcept
    {
        // TODO : fix
        return {};
    }

    void Quaternion::set_mat(const float4x4& mat) noexcept
    {
        float3 inversedScale
        {
            float3{ mat._00, mat._01, mat._02 }.len(),
            float3{ mat._10, mat._11, mat._12 }.len(),
            float3{ mat._20, mat._21, mat._22 }.len()
        };
        inversedScale.x = is_equal(inversedScale.x, 0.0f) ?  std::numeric_limits<float>::max() : 1.0f / inversedScale.x;
        inversedScale.y = is_equal(inversedScale.y, 0.0f) ?  std::numeric_limits<float>::max() : 1.0f / inversedScale.y;
        inversedScale.z = is_equal(inversedScale.z, 0.0f) ?  std::numeric_limits<float>::max() : 1.0f / inversedScale.z;
        float4x4 rotationMatrix
        {
            mat._00 * inversedScale.x, mat._01 * inversedScale.y, mat._02 * inversedScale.z, 0.0f,
            mat._10 * inversedScale.x, mat._11 * inversedScale.y, mat._12 * inversedScale.z, 0.0f,
            mat._20 * inversedScale.x, mat._21 * inversedScale.y, mat._22 * inversedScale.z, 0.0f,
            0.0f                     , 0.0f                     , 0.0f                     , 1.0f
        };
        set_rotation_mat(rotationMatrix);
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
        // https://www.cprogramming.com/tutorial/3d/quaternions.html
        return
        {
            {
                real * other.imaginary.x + imaginary.x * other.real         + imaginary.y * other.imaginary.z   - imaginary.z * other.imaginary.y,
                real * other.imaginary.y - imaginary.x * other.imaginary.z  + imaginary.y * other.real          + imaginary.z * other.imaginary.x,
                real * other.imaginary.z + imaginary.x * other.imaginary.y  - imaginary.y * other.imaginary.x   + imaginary.z * other.real
            },
            real * other.real - imaginary.x * other.imaginary.x - imaginary.y * other.imaginary.y - imaginary.z * other.imaginary.z
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

    Quaternion Quaternion::conjugate() const noexcept
    {
        Quaternion result;
        result.imaginary = imaginary * -1.0f;
        result.real = real;
        return result;
    }

    std::ostream& operator << (std::ostream& os, const Quaternion& quat) noexcept
    {
        os << quat.imaginary << " " << quat.real;
        return os;
    }
}

#endif
