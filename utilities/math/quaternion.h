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
    //
    // Quaternion conversions are taken from www.euclideanspace.com
    //
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
        , real{ 1.0f }
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
        const float halfAngle = to_radians(angle) * 0.5f;
        const float halfAnglesin = std::sin(halfAngle);
        x = axis.x * halfAnglesin;
        y = axis.y * halfAnglesin;
        z = axis.z * halfAnglesin;
        w = std::cos(halfAngle);
    }

    Quaternion::AxisAngle Quaternion::get_axis_angle() const noexcept
    {
        const float invSqrt = 1.0f / std::sqrt(1.0f - w * w);
        return
        {
            {
                x * invSqrt,
                y * invSqrt,
                z * invSqrt
            },
            to_degrees(2.0f * std::acos(w))
        };
    }

    void Quaternion::set_euler_angles(const float3& angles) noexcept
    {
        const float c1 = std::cos(to_radians(angles.y) * 0.5f);
        const float s1 = std::sin(to_radians(angles.y) * 0.5f);
        const float c2 = std::cos(to_radians(angles.z) * 0.5f);
        const float s2 = std::sin(to_radians(angles.z) * 0.5f);
        const float c3 = std::cos(to_radians(angles.x) * 0.5f);
        const float s3 = std::sin(to_radians(angles.x) * 0.5f);
        const float c1c2 = c1 * c2;
        const float s1s2 = s1 * s2;
        w = c1c2 * c3 - s1s2 * s3;
        x = c1c2 * s3 + s1s2 * c3;
        y = s1 * c2 * c3 + c1 * s2 * s3;
        z = c1 * s2 * c3 - s1 * c2 * s3;
    }

    float3 Quaternion::get_euler_angles() const noexcept
    {
        const float test = x * y + z * w;
        if (is_equal(test, 0.5f) || test > 0.5f)
        {
            // singularity at north pole
            float heading = 2.0f * std::atan2(x, w);
            float attitude = std::numbers::pi_v<float> * 0.5f;
            float bank = 0.0f;
            return
            {
                to_degrees(bank),
                to_degrees(heading),
                to_degrees(attitude)
            };
        }
        if (test < -0.499)
        {
            // singularity at south pole
            float heading = -2.0f * std::atan2(x, w);
            float attitude = -std::numbers::pi_v<float> * 0.5f;
            float bank = 0.0f;
            return
            {
                to_degrees(bank),
                to_degrees(heading),
                to_degrees(attitude)
            };
        }
        float sqx = x * x;
        float sqy = y * y;
        float sqz = z * z;
        float heading = std::atan2(2.0f * y * w - 2.0f * x * z, 1.0f - 2.0f * sqy - 2.0f * sqz);
        float attitude = std::asin(2.0f * test);
        float bank = std::atan2(2.0f * x * w - 2.0f * y * z, 1.0f - 2.0f * sqx - 2.0f * sqz);
        return
        {
            to_degrees(bank),
            to_degrees(heading),
            to_degrees(attitude)
        };
    }

    void Quaternion::set_rotation_mat(const float4x4& mat) noexcept
    {
        const float tr = mat._00 + mat._11 + mat._22;
        if (!is_equal(tr, 0.0f) && tr > 0.0f) { 
            float S = std::sqrt(tr + 1.0f) * 2.0f;
            w = 0.25f * S;
            x = (mat._21 - mat._12) / S;
            y = (mat._02 - mat._20) / S; 
            z = (mat._10 - mat._01) / S; 
        }
        else if ((mat._00 > mat._11) && (mat._00 > mat._22))
        { 
            float S = std::sqrt(1.0f + mat._00 - mat._11 - mat._22) * 2.0f;
            w = (mat._21 - mat._12) / S;
            x = 0.25f * S;
            y = (mat._01 + mat._10) / S; 
            z = (mat._02 + mat._20) / S; 
        }
        else if (mat._11 > mat._22)
        { 
            float S = std::sqrt(1.0f + mat._11 - mat._00 - mat._22) * 2.0f;
            w = (mat._02 - mat._20) / S;
            x = (mat._01 + mat._10) / S; 
            y = 0.25f * S;
            z = (mat._12 + mat._21) / S; 
        }
        else
        { 
            float S = std::sqrt(1.0f + mat._22 - mat._00 - mat._11) * 2.0f;
            w = (mat._10 - mat._01) / S;
            x = (mat._02 + mat._20) / S;
            y = (mat._12 + mat._21) / S;
            z = 0.25f * S;
        }
    }

    float4x4 Quaternion::get_rotation_mat() const noexcept
    {
        const float xx = x * x;
        const float xy = x * y;
        const float xz = x * z;
        const float xw = x * w;
        const float yy = y * y;
        const float yz = y * z;
        const float yw = y * w;
        const float zz = z * z;
        const float zw = z * w;
        const float m00 = 1.0f - 2.0f * (yy + zz);
        const float m01 =        2.0f * (xy - zw);
        const float m02 =        2.0f * (xz + yw);
        const float m10 =        2.0f * (xy + zw);
        const float m11 = 1.0f - 2.0f * (xx + zz);
        const float m12 =        2.0f * (yz - xw);
        const float m20 =        2.0f * (xz - yw);
        const float m21 =        2.0f * (yz + xw);
        const float m22 = 1.0f - 2.0f * (xx + yy);
        return
        {
            m00, m01, m02, 0,
            m10, m11, m12, 0,
            m20, m21, m22, 0,
            0  , 0  , 0  , 1
        };
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
