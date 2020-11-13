#ifndef AL_QUATERNION_H
#define AL_QUATERNION_H

#include <cmath>
#include <numbers>

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

        friend std::ostream& operator << (std::ostream& os, const Quaternion& quat) noexcept;

    public:
        union
        {
            struct
            {
                float3 imaginary;
                float real;
            };
            struct
            {
                float x, y, z, w;
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

    void Quaternion::set_euler_angles(const float3& angles) noexcept
    {
        // https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
        // xyz - zxy

        // @NOTE : PHB to PYR notations conversion
        //         pitch - x - pitch
        //         head  - y - yaw
        //         bank  - z - roll

        float cy = std::cos(to_radians(angles.y) * 0.5f);
        float sy = std::sin(to_radians(angles.y) * 0.5f);
        float cx = std::cos(to_radians(angles.x) * 0.5f);
        float sx = std::sin(to_radians(angles.x) * 0.5f);
        float cz = std::cos(to_radians(angles.z) * 0.5f);
        float sz = std::sin(to_radians(angles.z) * 0.5f);

        x = cz * sx * cy + sz * cx * sy;
        y = cz * cx * sy - sz * sx * cy;
        z = sz * cx * cy - cz * sx * sy;
        w = cz * cx * cy + sz * sx * sy;
    }

    float3 Quaternion::get_euler_angles() const noexcept
    {
        // https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
        // xyz - zxy

        float3 result;
        float sinp = 2.0f * (w * x - y * z);
        if (std::abs(sinp) >= 1.0f)
        {
            result.x = to_degrees(std::copysign(std::numbers::pi_v<float> / 2.0f, sinp));
        }
        else
        {
            result.x = to_degrees(std::asin(sinp));
        }
        result.y = to_degrees(std::atan2(2.0f * (w * y + z * x), 1.0f - 2.0f * (x * x + y * y)));
        result.z = to_degrees(std::atan2(2.0f * (w * z + x * y), 1.0f - 2.0f * (z * z + x * x)));
        return result;
    }

    void Quaternion::set_rotation_mat(const float4x4& mat) noexcept
    {
        // https://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToQuaternion/index.htm

        w = std::sqrt(1.0f + mat.m[0][0] + mat.m[1][1] + mat.m[2][2]) / 2.0f;
        x = (mat.m[2][1] - mat.m[1][2]) / (4.0f * w);
        y = (mat.m[0][2] - mat.m[2][0]) / (4.0f * w);
        z = (mat.m[1][0] - mat.m[0][1]) / (4.0f * w);
    }

    float4x4 Quaternion::get_rotation_mat() const noexcept
    {
        // https://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToMatrix/index.htm

        return
        {
            1.0f - 2.0f * y * y - 2.0f * z * z  , 2.0f * x * y - 2.0f * z * w       , 2.0f * x * z + 2.0f * y * w       , 0.0f,
            2.0f * x * y + 2.0f * z * w         , 1.0f - 2.0f * x * x - 2.0f * z * z, 2.0f * y * z + 2.0f * x * w       , 0.0f,
            2.0f * x * z - 2.0f * y * w         , 2.0f * y * z + 2.0f * x * w       , 1.0f - 2.0f * x * x - 2.0f * y * y, 0.0f,
            0.0f                                , 0.0f                              , 0.0f                              , 1.0f
        };
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

    std::ostream& operator << (std::ostream& os, const Quaternion& quat) noexcept
    {
        os << quat.imaginary << " " << quat.real;
        return os;
    }
}

#endif
