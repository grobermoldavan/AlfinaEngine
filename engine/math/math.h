#ifndef AL_MATH_H
#define AL_MATH_H

#include <type_traits>  // for std::is_floating_point and std::is_integral
#include <cmath>        // for std::sqrt and trigonometry
#include <numbers>      // for std::numbers
#include <limits>       // for std::numeric_limits

#include "engine/types.h"

namespace al
{
    template<typename Type>
    concept math_number = std::is_floating_point<Type>::value || std::is_integral<Type>::value;

    template<math_number T>
    constexpr T to_radians(T degrees)
    {
        return degrees * std::numbers::pi_v<T> / T{ 180 };
    }

    template<math_number T>
    constexpr T to_degrees(T radians)
    {
        return radians * T{ 180 } / std::numbers::pi_v<T>;
    }

    template<math_number T>
    constexpr bool is_equal(T value1, T value2, T precision = std::numeric_limits<T>::epsilon())
    {
        return (value1 > value2 ? value1 - value2 : value2 - value1) < precision;
    }

    // ===============================================================================================
    // Vectors
    // ===============================================================================================

    template<math_number Type>
    struct vec2
    {
        union
        {
            struct 
            {
                Type x;
                Type y;
            };
            struct 
            {
                Type u;
                Type v;
            };
            Type elements[2];
        };
    };

    template<math_number Type> Type         v_dot(const vec2<Type>& first, const vec2<Type>& second)    { return first.x * second.x + first.y * second.y; }
    template<math_number Type> vec2<Type>   v_add(const vec2<Type>& first, const vec2<Type>& second)    { return { first.x + second.x, first.y + second.y }; }
    template<math_number Type> vec2<Type>   v_sub(const vec2<Type>& first, const vec2<Type>& second)    { return { first.x - second.x, first.y - second.y }; }
    template<math_number Type> vec2<Type>   v_mul(const vec2<Type>& vec, Type value)                    { return { vec.x * value, vec.y * value }; }
    template<math_number Type> vec2<Type>   v_div(const vec2<Type>& vec, Type value)                    { return { vec.x / value, vec.y / value }; }
    template<math_number Type> f32          v_len(const vec2<Type>& vec)                                { return std::sqrt(vec.x * vec.x + vec.y * vec.y); }
    template<math_number Type> f32          v_len2(const vec2<Type>& vec)                               { return vec.x * vec.x + vec.y * vec.y; }
    template<math_number Type> vec2<Type>   v_normalized(const vec2<Type>& vec)                         { f32 length = v_len(vec); return { vec.x / length, vec.y / length }; }

    template<math_number Type>
    struct vec3
    {
        union
        {
            struct 
            {
                Type x;
                Type y;
                Type z;
            };
            struct 
            {
                Type r;
                Type g;
                Type b;
            };
            Type elements[3];
        };
    };

    template<math_number Type> Type         v_dot(const vec3<Type>& first, const vec3<Type>& second)    { return first.x * second.x + first.y * second.y + first.z * second.z; }
    template<math_number Type> vec3<Type>   v_add(const vec3<Type>& first, const vec3<Type>& second)    { return { first.x + second.x, first.y + second.y, first.z + second.z }; }
    template<math_number Type> vec3<Type>   v_sub(const vec3<Type>& first, const vec3<Type>& second)    { return { first.x - second.x, first.y - second.y, first.z - second.z }; }
    template<math_number Type> vec3<Type>   v_mul(const vec3<Type>& vec, Type value)                    { return { vec.x * value, vec.y * value, vec.z * value }; }
    template<math_number Type> vec3<Type>   v_div(const vec3<Type>& vec, Type value)                    { return { vec.x / value, vec.y / value, vec.z / value }; }
    template<math_number Type> f32          v_len(const vec3<Type>& vec)                                { return std::sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z); }
    template<math_number Type> f32          v_len2(const vec3<Type>& vec)                               { return vec.x * vec.x + vec.y * vec.y + vec.z * vec.z; }
    template<math_number Type> vec3<Type>   v_normalized(const vec3<Type>& vec)                         { f32 length = v_len(vec); return { vec.x / length, vec.y / length, vec.z / length }; }
    template<math_number Type> vec3<Type>   v_cross(const vec3<Type>& a, const vec3<Type>& b)           { return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x }; }

    template<math_number Type>
    struct vec4
    {
        union
        {
            struct 
            {
                Type x;
                Type y;
                Type z;
                Type w;
            };
            struct 
            {
                Type r;
                Type g;
                Type b;
                Type a;
            };
            Type elements[4];
        };
    };

    template<math_number Type> Type         v_dot(const vec4<Type>& first, const vec4<Type>& second)    { return first.x * second.x + first.y * second.y + first.z * second.z + first.w * second.w; }
    template<math_number Type> vec4<Type>   v_add(const vec4<Type>& first, const vec4<Type>& second)    { return { first.x + second.x, first.y + second.y, first.z + second.z, first.w + second.w }; }
    template<math_number Type> vec4<Type>   v_sub(const vec4<Type>& first, const vec4<Type>& second)    { return { first.x - second.x, first.y - second.y, first.z - second.z, first.w - second.w }; }
    template<math_number Type> vec4<Type>   v_mul(const vec4<Type>& vec, Type value)                    { return { vec.x * value, vec.y * value, vec.z * value, vec.w * value }; }
    template<math_number Type> vec4<Type>   v_div(const vec4<Type>& vec, Type value)                    { return { vec.x / value, vec.y / value, vec.z / value, vec.w / value }; }
    template<math_number Type> f32          v_len(const vec4<Type>& vec)                                { return std::sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z + vec.w * vec.w); }
    template<math_number Type> f32          v_len2(const vec4<Type>& vec)                               { return vec.x * vec.x + vec.y * vec.y + vec.z * vec.z + vec.w * vec.w; }
    template<math_number Type> vec4<Type>   v_normalized(const vec4<Type>& vec)                         { f32 length = v_len(vec); return { vec.x / length, vec.y / length, vec.z / length, vec.w / length }; }
    template<math_number Type> vec4<Type>   v_cross(const vec4<Type>& a, const vec4<Type>& b)           { return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x }; }

    using f32_2 = vec2<f32>;
    using f32_3 = vec3<f32>;
    using f32_4 = vec4<f32>;

    using f64_2 = vec2<f64>;
    using f64_3 = vec3<f64>;
    using f64_4 = vec4<f64>;

    using s32_2 = vec2<s32>;
    using s32_3 = vec3<s32>;
    using s32_4 = vec4<s32>;

    using u32_2 = vec2<u32>;
    using u32_3 = vec3<u32>;
    using u32_4 = vec4<u32>;

    // ===============================================================================================
    // Matricies
    // ===============================================================================================

    template<math_number Type>
    struct mat4x4
    {
        union
        {
            struct
            {
                Type _00, _01, _02, _03;
                Type _10, _11, _12, _13;
                Type _20, _21, _22, _23;
                Type _30, _31, _32, _33;
            };
            
            vec4<Type> rows[4];
            Type elements[16];
            Type m[4][4];
        };
    };

    template<math_number Type>
    mat4x4<Type> m_mul(const mat4x4<Type>& first, const mat4x4<Type>& second)
    {
        return
        {
            (second.m[0][0] * first.m[0][0]) + (second.m[1][0] * first.m[0][1]) + (second.m[2][0] * first.m[0][2]) + (second.m[3][0] * first.m[0][3]),  // 0 0
            (second.m[0][1] * first.m[0][0]) + (second.m[1][1] * first.m[0][1]) + (second.m[2][1] * first.m[0][2]) + (second.m[3][1] * first.m[0][3]),  // 0 1
            (second.m[0][2] * first.m[0][0]) + (second.m[1][2] * first.m[0][1]) + (second.m[2][2] * first.m[0][2]) + (second.m[3][2] * first.m[0][3]),  // 0 2
            (second.m[0][3] * first.m[0][0]) + (second.m[1][3] * first.m[0][1]) + (second.m[2][3] * first.m[0][2]) + (second.m[3][3] * first.m[0][3]),  // 0 3
            (second.m[0][0] * first.m[1][0]) + (second.m[1][0] * first.m[1][1]) + (second.m[2][0] * first.m[1][2]) + (second.m[3][0] * first.m[1][3]),  // 1 0
            (second.m[0][1] * first.m[1][0]) + (second.m[1][1] * first.m[1][1]) + (second.m[2][1] * first.m[1][2]) + (second.m[3][1] * first.m[1][3]),  // 1 1
            (second.m[0][2] * first.m[1][0]) + (second.m[1][2] * first.m[1][1]) + (second.m[2][2] * first.m[1][2]) + (second.m[3][2] * first.m[1][3]),  // 1 2
            (second.m[0][3] * first.m[1][0]) + (second.m[1][3] * first.m[1][1]) + (second.m[2][3] * first.m[1][2]) + (second.m[3][3] * first.m[1][3]),  // 1 3
            (second.m[0][0] * first.m[2][0]) + (second.m[1][0] * first.m[2][1]) + (second.m[2][0] * first.m[2][2]) + (second.m[3][0] * first.m[2][3]),  // 2 0
            (second.m[0][1] * first.m[2][0]) + (second.m[1][1] * first.m[2][1]) + (second.m[2][1] * first.m[2][2]) + (second.m[3][1] * first.m[2][3]),  // 2 1
            (second.m[0][2] * first.m[2][0]) + (second.m[1][2] * first.m[2][1]) + (second.m[2][2] * first.m[2][2]) + (second.m[3][2] * first.m[2][3]),  // 2 2
            (second.m[0][3] * first.m[2][0]) + (second.m[1][3] * first.m[2][1]) + (second.m[2][3] * first.m[2][2]) + (second.m[3][3] * first.m[2][3]),  // 2 3
            (second.m[0][0] * first.m[3][0]) + (second.m[1][0] * first.m[3][1]) + (second.m[2][0] * first.m[3][2]) + (second.m[3][0] * first.m[3][3]),  // 3 0
            (second.m[0][1] * first.m[3][0]) + (second.m[1][1] * first.m[3][1]) + (second.m[2][1] * first.m[3][2]) + (second.m[3][1] * first.m[3][3]),  // 3 1
            (second.m[0][2] * first.m[3][0]) + (second.m[1][2] * first.m[3][1]) + (second.m[2][2] * first.m[3][2]) + (second.m[3][2] * first.m[3][3]),  // 3 2
            (second.m[0][3] * first.m[3][0]) + (second.m[1][3] * first.m[3][1]) + (second.m[2][3] * first.m[3][2]) + (second.m[3][3] * first.m[3][3])   // 3 3
        };
    }

    template<math_number Type>
    mat4x4<Type> m_mul(const mat4x4<Type>& mat, Type value)
    {
        return
        {
            mat.m[0][0] * value, mat.m[0][1] * value, mat.m[0][2] * value, mat.m[0][3] * value,
            mat.m[1][0] * value, mat.m[1][1] * value, mat.m[1][2] * value, mat.m[1][3] * value,
            mat.m[2][0] * value, mat.m[2][1] * value, mat.m[2][2] * value, mat.m[2][3] * value,
            mat.m[3][0] * value, mat.m[3][1] * value, mat.m[3][2] * value, mat.m[3][3] * value
        };
    }

    template<math_number Type>
    vec4<Type> m_mul(const mat4x4<Type>& mat, const vec4<Type>& value)
    {
        return
        {
            value.x * mat.m[0][0] + value.y * mat.m[0][1] + value.z * mat.m[0][2] + value.w * mat.m[0][3],
            value.x * mat.m[1][0] + value.y * mat.m[1][1] + value.z * mat.m[1][2] + value.w * mat.m[1][3],
            value.x * mat.m[2][0] + value.y * mat.m[2][1] + value.z * mat.m[2][2] + value.w * mat.m[2][3],
            value.x * mat.m[3][0] + value.y * mat.m[3][1] + value.z * mat.m[3][2] + value.w * mat.m[3][3]
        };
    }

    template<math_number Type>
    mat4x4<Type> m_transposed(const mat4x4<Type>& mat)
    {
        return
        {
            mat._00, mat._10, mat._20, mat._30,
            mat._01, mat._11, mat._21, mat._31,
            mat._02, mat._12, mat._22, mat._32,
            mat._03, mat._13, mat._23, mat._33
        };
    }

    template<math_number Type>
    Type m_det(const mat4x4<Type>& mat)
    {
        // 2x2 sub-determinants
        const f32 det2_01_01 = mat.m[0][0] * mat.m[1][1] - mat.m[0][1] * mat.m[1][0];
        const f32 det2_01_02 = mat.m[0][0] * mat.m[1][2] - mat.m[0][2] * mat.m[1][0];
        const f32 det2_01_03 = mat.m[0][0] * mat.m[1][3] - mat.m[0][3] * mat.m[1][0];
        const f32 det2_01_12 = mat.m[0][1] * mat.m[1][2] - mat.m[0][2] * mat.m[1][1];
        const f32 det2_01_13 = mat.m[0][1] * mat.m[1][3] - mat.m[0][3] * mat.m[1][1];
        const f32 det2_01_23 = mat.m[0][2] * mat.m[1][3] - mat.m[0][3] * mat.m[1][2];
        // 3x3 sub-determinants
        const f32 det3_201_012 = mat.m[2][0] * det2_01_12 - mat.m[2][1] * det2_01_02 + mat.m[2][2] * det2_01_01;
        const f32 det3_201_013 = mat.m[2][0] * det2_01_13 - mat.m[2][1] * det2_01_03 + mat.m[2][3] * det2_01_01;
        const f32 det3_201_023 = mat.m[2][0] * det2_01_23 - mat.m[2][2] * det2_01_03 + mat.m[2][3] * det2_01_02;
        const f32 det3_201_123 = mat.m[2][1] * det2_01_23 - mat.m[2][2] * det2_01_13 + mat.m[2][3] * det2_01_12;
        return Type{ -1 } * det3_201_123 * mat.m[3][0] + det3_201_023 * mat.m[3][1] - det3_201_013 * mat.m[3][2] + det3_201_012 * mat.m[3][3];
    }

    template<math_number Type>
    mat4x4<Type> m_inverted(const mat4x4<Type>& mat)
    {
        // https://stackoverflow.com/questions/1148309/inverting-a-4x4-matrix
        // https://github.com/willnode/N-Matrix-Programmer
        const Type determinant = m_det(mat);
        if (is_equal(determinant, Type{ 0 }))
        {
            // @TODO : add method which returns bool here
            // Can't invert
            return { };
        }
        const Type invDet = Type{ 1 } / determinant;
        const Type A2323 = mat.m[2][2] * mat.m[3][3] - mat.m[2][3] * mat.m[3][2];
        const Type A1323 = mat.m[2][1] * mat.m[3][3] - mat.m[2][3] * mat.m[3][1];
        const Type A1223 = mat.m[2][1] * mat.m[3][2] - mat.m[2][2] * mat.m[3][1];
        const Type A0323 = mat.m[2][0] * mat.m[3][3] - mat.m[2][3] * mat.m[3][0];
        const Type A0223 = mat.m[2][0] * mat.m[3][2] - mat.m[2][2] * mat.m[3][0];
        const Type A0123 = mat.m[2][0] * mat.m[3][1] - mat.m[2][1] * mat.m[3][0];
        const Type A2313 = mat.m[1][2] * mat.m[3][3] - mat.m[1][3] * mat.m[3][2];
        const Type A1313 = mat.m[1][1] * mat.m[3][3] - mat.m[1][3] * mat.m[3][1];
        const Type A1213 = mat.m[1][1] * mat.m[3][2] - mat.m[1][2] * mat.m[3][1];
        const Type A2312 = mat.m[1][2] * mat.m[2][3] - mat.m[1][3] * mat.m[2][2];
        const Type A1312 = mat.m[1][1] * mat.m[2][3] - mat.m[1][3] * mat.m[2][1];
        const Type A1212 = mat.m[1][1] * mat.m[2][2] - mat.m[1][2] * mat.m[2][1];
        const Type A0313 = mat.m[1][0] * mat.m[3][3] - mat.m[1][3] * mat.m[3][0];
        const Type A0213 = mat.m[1][0] * mat.m[3][2] - mat.m[1][2] * mat.m[3][0];
        const Type A0312 = mat.m[1][0] * mat.m[2][3] - mat.m[1][3] * mat.m[2][0];
        const Type A0212 = mat.m[1][0] * mat.m[2][2] - mat.m[1][2] * mat.m[2][0];
        const Type A0113 = mat.m[1][0] * mat.m[3][1] - mat.m[1][1] * mat.m[3][0];
        const Type A0112 = mat.m[1][0] * mat.m[2][1] - mat.m[1][1] * mat.m[2][0];
        return
        {
            invDet              * (mat.m[1][1] * A2323 - mat.m[1][2] * A1323 + mat.m[1][3] * A1223),    // 0 0
            invDet * Type{ -1 } * (mat.m[0][1] * A2323 - mat.m[0][2] * A1323 + mat.m[0][3] * A1223),    // 0 1
            invDet              * (mat.m[0][1] * A2313 - mat.m[0][2] * A1313 + mat.m[0][3] * A1213),    // 0 2
            invDet * Type{ -1 } * (mat.m[0][1] * A2312 - mat.m[0][2] * A1312 + mat.m[0][3] * A1212),    // 0 3
            invDet * Type{ -1 } * (mat.m[1][0] * A2323 - mat.m[1][2] * A0323 + mat.m[1][3] * A0223),    // 1 0
            invDet              * (mat.m[0][0] * A2323 - mat.m[0][2] * A0323 + mat.m[0][3] * A0223),    // 1 1
            invDet * Type{ -1 } * (mat.m[0][0] * A2313 - mat.m[0][2] * A0313 + mat.m[0][3] * A0213),    // 1 2
            invDet              * (mat.m[0][0] * A2312 - mat.m[0][2] * A0312 + mat.m[0][3] * A0212),    // 1 3
            invDet              * (mat.m[1][0] * A1323 - mat.m[1][1] * A0323 + mat.m[1][3] * A0123),    // 2 0
            invDet * Type{ -1 } * (mat.m[0][0] * A1323 - mat.m[0][1] * A0323 + mat.m[0][3] * A0123),    // 2 1
            invDet              * (mat.m[0][0] * A1313 - mat.m[0][1] * A0313 + mat.m[0][3] * A0113),    // 2 2
            invDet * Type{ -1 } * (mat.m[0][0] * A1312 - mat.m[0][1] * A0312 + mat.m[0][3] * A0112),    // 2 3
            invDet * Type{ -1 } * (mat.m[1][0] * A1223 - mat.m[1][1] * A0223 + mat.m[1][2] * A0123),    // 3 0
            invDet              * (mat.m[0][0] * A1223 - mat.m[0][1] * A0223 + mat.m[0][2] * A0123),    // 3 1
            invDet * Type{ -1 } * (mat.m[0][0] * A1213 - mat.m[0][1] * A0213 + mat.m[0][2] * A0113),    // 3 2
            invDet              * (mat.m[0][0] * A1212 - mat.m[0][1] * A0212 + mat.m[0][2] * A0112)     // 3 3
        };
    }
    
    using f32_4x4 = mat4x4<f32>;

    constexpr f32_4x4 IDENTITY4
    {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };

    template<math_number Type>
    mat4x4<Type> m_translation(const vec3<Type>& position)
    {
        return
        {
            1, 0, 0, position.x,
            0, 1, 0, position.y,
            0, 0, 1, position.z,
            0, 0, 0, 1,
        };
    }

    template<math_number Type>
    mat4x4<Type> m_rotation(const vec3<Type>& eulerAngles)
    {
        const Type cosPitch = static_cast<Type>(std::cos(to_radians(eulerAngles.x)));
        const Type sinPitch = static_cast<Type>(std::sin(to_radians(eulerAngles.x)));
        const Type cosYaw   = static_cast<Type>(std::cos(to_radians(eulerAngles.y)));
        const Type sinYaw   = static_cast<Type>(std::sin(to_radians(eulerAngles.y)));
        const Type cosRoll  = static_cast<Type>(std::cos(to_radians(eulerAngles.z)));
        const Type sinRoll  = static_cast<Type>(std::sin(to_radians(eulerAngles.z)));
        const mat4x4<Type> pitch
        {
            1, 0       , 0        , 0,
            0, cosPitch, -sinPitch, 0,
            0, sinPitch,  cosPitch, 0,
            0, 0       , 0        , 1
        };
        const mat4x4<Type> yaw
        {
            cosYaw , 0, sinYaw, 0,
            0      , 1, 0     , 0,
            -sinYaw, 0, cosYaw, 0,
            0      , 0, 0     , 1
        };
        const mat4x4<Type> roll
        {
            cosRoll, -sinRoll, 0, 0,
            sinRoll,  cosRoll, 0, 0,
            0      , 0       , 1, 0,
            0      , 0       , 0, 1
        };
        return m_mul(yaw, m_mul(roll, pitch));
    }

    template<math_number Type>
    mat4x4<Type> m_scale(const vec3<Type>& scale)
    {
        return
        {
            scale.x , 0         , 0         , 0,
            0       , scale.y   , 0         , 0,
            0       , 0         , scale.z   , 0,
            0       , 0         , 0         , 1
        };
    }

    // ===============================================================================================
    // Quaternion
    // ===============================================================================================

    //
    // Quaternion conversions are taken from www.euclideanspace.com
    //
    // @TODO :  optimize conversions
    //
    struct Quaternion
    {
        union
        {
            struct
            {
                f32 real;
                f32_3 imaginary;
            };
            struct
            {
                f32 w, x, y, z;
            };
            f32 elements[4];
        };
    };

    struct AxisAngle
    {
        f32_3 axis;
        f32 angle;
    };

    Quaternion q_conjugate(const Quaternion& quat) // inverted
    {
        return
        {
            quat.real,
            v_mul(quat.imaginary, -1.0f)
        };
    }

    Quaternion q_from_axis_angle(const AxisAngle& axisAngle)
    {
        const f32 halfAngle = to_radians(axisAngle.angle) * 0.5f;
        const f32 halfAnglesin = std::sin(halfAngle);
        return
        {
            .w = std::cos(halfAngle),
            .x = axisAngle.axis.x * halfAnglesin,
            .y = axisAngle.axis.y * halfAnglesin,
            .z = axisAngle.axis.z * halfAnglesin
        };
    }

    AxisAngle q_to_axis_angle(const Quaternion& quat)
    {
        const f32 invSqrt = 1.0f / std::sqrt(1.0f - quat.w * quat.w);
        return
        {
            {
                quat.x * invSqrt,
                quat.y * invSqrt,
                quat.z * invSqrt
            },
            to_degrees(2.0f * std::acos(quat.w))
        };
    }

    Quaternion q_from_euler_angles(const f32_3& angles)
    {
        const f32 halfY = to_radians(angles.y) * 0.5f;
        const f32 halfZ = to_radians(angles.z) * 0.5f;
        const f32 halfX = to_radians(angles.x) * 0.5f;
        const f32 c1 = std::cos(halfY);
        const f32 s1 = std::sin(halfY);
        const f32 c2 = std::cos(halfZ);
        const f32 s2 = std::sin(halfZ);
        const f32 c3 = std::cos(halfX);
        const f32 s3 = std::sin(halfX);
        const f32 c1c2 = c1 * c2;
        const f32 s1s2 = s1 * s2;
        return
        {
            .w = c1c2 * c3 - s1s2 * s3,
            .x = c1c2 * s3 + s1s2 * c3,
            .y = s1 * c2 * c3 + c1 * s2 * s3,
            .z = c1 * s2 * c3 - s1 * c2 * s3
        };
    }

    f32_3 q_to_euler_angles(const Quaternion& quat)
    {
        const f32 test = quat.x * quat.y + quat.z * quat.w;
        if (is_equal(test, 0.5f) || test > 0.5f)
        {
            // singularity at north pole
            f32 heading = 2.0f * std::atan2(quat.x, quat.w);
            f32 attitude = std::numbers::pi_v<f32> * 0.5f;
            f32 bank = 0.0f;
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
            f32 heading = -2.0f * std::atan2(quat.x, quat.w);
            f32 attitude = -std::numbers::pi_v<f32> * 0.5f;
            f32 bank = 0.0f;
            return
            {
                to_degrees(bank),
                to_degrees(heading),
                to_degrees(attitude)
            };
        }
        f32 sqx = quat.x * quat.x;
        f32 sqy = quat.y * quat.y;
        f32 sqz = quat.z * quat.z;
        f32 heading = std::atan2(2.0f * quat.y * quat.w - 2.0f * quat.x * quat.z, 1.0f - 2.0f * sqy - 2.0f * sqz);
        f32 attitude = std::asin(2.0f * test);
        f32 bank = std::atan2(2.0f * quat.x * quat.w - 2.0f * quat.y * quat.z, 1.0f - 2.0f * sqx - 2.0f * sqz);
        return
        {
            to_degrees(bank),
            to_degrees(heading),
            to_degrees(attitude)
        };
    }

    Quaternion q_from_rotation_mat(const f32_4x4& mat)
    {
        const f32 tr = mat._00 + mat._11 + mat._22;
        if (!is_equal(tr, 0.0f) && tr > 0.0f)
        { 
            f32 S = std::sqrt(tr + 1.0f) * 2.0f;
            return
            {
                .w = 0.25f * S,
                .x = (mat._21 - mat._12) / S,
                .y = (mat._02 - mat._20) / S,
                .z = (mat._10 - mat._01) / S
            };
        }
        else if ((mat._00 > mat._11) && (mat._00 > mat._22))
        { 
            f32 S = std::sqrt(1.0f + mat._00 - mat._11 - mat._22) * 2.0f;
            return
            {
                .w = (mat._21 - mat._12) / S,
                .x = 0.25f * S,
                .y = (mat._01 + mat._10) / S,
                .z = (mat._02 + mat._20) / S
            };
        }
        else if (mat._11 > mat._22)
        { 
            f32 S = std::sqrt(1.0f + mat._11 - mat._00 - mat._22) * 2.0f;
            return
            {
                .w = (mat._02 - mat._20) / S,
                .x = (mat._01 + mat._10) / S,
                .y = 0.25f * S,
                .z = (mat._12 + mat._21) / S
            };
        }
        else
        { 
            f32 S = std::sqrt(1.0f + mat._22 - mat._00 - mat._11) * 2.0f;
            return
            {
                .w = (mat._10 - mat._01) / S,
                .x = (mat._02 + mat._20) / S,
                .y = (mat._12 + mat._21) / S,
                .z = 0.25f * S
            };
        }
    }

    f32_4x4 q_to_rotation_mat(const Quaternion& quat)
    {
        const f32 xx = quat.x * quat.x;
        const f32 xy = quat.x * quat.y;
        const f32 xz = quat.x * quat.z;
        const f32 xw = quat.x * quat.w;
        const f32 yy = quat.y * quat.y;
        const f32 yz = quat.y * quat.z;
        const f32 yw = quat.y * quat.w;
        const f32 zz = quat.z * quat.z;
        const f32 zw = quat.z * quat.w;
        const f32 m00 = 1.0f - 2.0f * (yy + zz);
        const f32 m01 =        2.0f * (xy - zw);
        const f32 m02 =        2.0f * (xz + yw);
        const f32 m10 =        2.0f * (xy + zw);
        const f32 m11 = 1.0f - 2.0f * (xx + zz);
        const f32 m12 =        2.0f * (yz - xw);
        const f32 m20 =        2.0f * (xz - yw);
        const f32 m21 =        2.0f * (yz + xw);
        const f32 m22 = 1.0f - 2.0f * (xx + yy);
        return
        {
            m00, m01, m02, 0,
            m10, m11, m12, 0,
            m20, m21, m22, 0,
            0  , 0  , 0  , 1
        };
    }

    Quaternion q_from_mat(const f32_4x4& mat)
    {
        f32_3 inversedScale
        {
            v_len(f32_3{ mat._00, mat._01, mat._02 }),
            v_len(f32_3{ mat._10, mat._11, mat._12 }),
            v_len(f32_3{ mat._20, mat._21, mat._22 })
        };
        inversedScale.x = is_equal(inversedScale.x, 0.0f) ?  std::numeric_limits<f32>::max() : 1.0f / inversedScale.x;
        inversedScale.y = is_equal(inversedScale.y, 0.0f) ?  std::numeric_limits<f32>::max() : 1.0f / inversedScale.y;
        inversedScale.z = is_equal(inversedScale.z, 0.0f) ?  std::numeric_limits<f32>::max() : 1.0f / inversedScale.z;
        f32_4x4 rotationMatrix
        {
            mat._00 * inversedScale.x, mat._01 * inversedScale.y, mat._02 * inversedScale.z, 0.0f,
            mat._10 * inversedScale.x, mat._11 * inversedScale.y, mat._12 * inversedScale.z, 0.0f,
            mat._20 * inversedScale.x, mat._21 * inversedScale.y, mat._22 * inversedScale.z, 0.0f,
            0.0f                     , 0.0f                     , 0.0f                     , 1.0f
        };
        return q_from_rotation_mat(rotationMatrix);
    }

    Quaternion q_add(const Quaternion& first, const Quaternion& second)
    {
        return
        {
            first.real + second.real,
            v_add(first.imaginary, second.imaginary)
        };
    }

    Quaternion q_sub(const Quaternion& first, const Quaternion& second)
    {
        return
        {
            first.real + second.real,
            v_sub(first.imaginary, second.imaginary)
        };
    }

    Quaternion q_mul(const Quaternion& first, const Quaternion& second)
    {
        // https://www.cprogramming.com/tutorial/3d/quaternions.html
        return
        {
            first.real * second.real - first.imaginary.x * second.imaginary.x - first.imaginary.y * second.imaginary.y - first.imaginary.z * second.imaginary.z,
            {
                first.real * second.imaginary.x + first.imaginary.x * second.real         + first.imaginary.y * second.imaginary.z   - first.imaginary.z * second.imaginary.y,
                first.real * second.imaginary.y - first.imaginary.x * second.imaginary.z  + first.imaginary.y * second.real          + first.imaginary.z * second.imaginary.x,
                first.real * second.imaginary.z + first.imaginary.x * second.imaginary.y  - first.imaginary.y * second.imaginary.x   + first.imaginary.z * second.real
            }
        };
    }

    Quaternion q_mul(const Quaternion& quat, f32 scalar)
    {
        return
        {
            quat.real * scalar,
            v_mul(quat.imaginary, scalar)
        };
    }

    Quaternion q_div(const Quaternion& first, const Quaternion& second)
    {
        return q_mul(first, q_conjugate(second));
    }

    Quaternion q_div(const Quaternion& quat, f32 scalar)
    {
        return
        {
            quat.real / scalar,
            v_div(quat.imaginary, scalar)
        };
    }

    f32 q_len(const Quaternion& quat)
    {
        return std::sqrt(quat.real * quat.real + v_len2(quat.imaginary));
    }

    f32 q_len2(const Quaternion& quat)
    {
        return quat.real * quat.real + v_len2(quat.imaginary);
    }

    Quaternion q_normalized(const Quaternion& quat)
    {
        const f32 length = q_len(quat);
        return
        {
            quat.real / length,
            v_div(quat.imaginary, length)
        };
    }

    constexpr Quaternion IDENTITY_QUAT
    {
        .real = 1.0f,
        .imaginary = { 0, 0, 0 }
    };

    // ===============================================================================================
    // Transform
    // ===============================================================================================

    struct Transform
    {
        f32_4x4 matrix = IDENTITY4;
    };

    f32_3 t_get_position(const Transform& trf)
    {
        return { trf.matrix._03, trf.matrix._13, trf.matrix._23 };
    }

    f32_3 t_get_rotation(const Transform& trf)
    {
        return q_to_euler_angles(q_from_mat(trf.matrix));
    }

    f32_3 t_get_scale(const Transform& trf)
    {
        return
        {
            v_len(f32_3{ trf.matrix._00, trf.matrix._01, trf.matrix._02 }),
            v_len(f32_3{ trf.matrix._10, trf.matrix._11, trf.matrix._12 }),
            v_len(f32_3{ trf.matrix._20, trf.matrix._21, trf.matrix._22 })
        };
    }

    f32_3 t_get_forward(const Transform& trf)
    {
        return v_normalized(f32_3{ trf.matrix.m[0][2], trf.matrix.m[1][2], trf.matrix.m[2][2] });
    }

    f32_3 t_get_right(const Transform& trf)
    {
        return v_normalized(f32_3{ trf.matrix.m[0][0], trf.matrix.m[1][0], trf.matrix.m[2][0] });
    }

    f32_3 t_get_up(const Transform& trf)
    {
        return v_normalized(f32_3{ trf.matrix.m[0][1], trf.matrix.m[1][1], trf.matrix.m[2][1] });
    }

    f32_3 t_get_inverted_scale(const Transform& trf)
    {
        f32_3 scale = t_get_scale(trf);
        return
        {
            is_equal(scale.x, 0.0f) ?  (std::numeric_limits<f32>::max)() : 1.0f / scale.x,
            is_equal(scale.y, 0.0f) ?  (std::numeric_limits<f32>::max)() : 1.0f / scale.y,
            is_equal(scale.z, 0.0f) ?  (std::numeric_limits<f32>::max)() : 1.0f / scale.z
        };
    }

    void t_set_position(Transform* trf, const f32_3& position)
    {
        trf->matrix._03 = position.x;
        trf->matrix._13 = position.y;
        trf->matrix._23 = position.z;
    }

    void t_set_rotation(Transform* trf, const f32_3& eulerAngles)
    {
        f32_4x4 tr = m_translation(t_get_position(*trf));
        f32_4x4 rt = m_rotation(eulerAngles); 
        f32_4x4 sc = m_scale(t_get_scale(*trf));
        trf->matrix = m_mul(tr, m_mul(rt, sc));
    }

    void t_set_scale(Transform* trf, const f32_3& scale)
    {
        f32_4x4 tr = m_translation(t_get_position(*trf));
        f32_4x4 rt = m_rotation(t_get_rotation(*trf)); 
        f32_4x4 sc = m_scale(scale);
        trf->matrix = m_mul(tr, m_mul(rt, sc));
        // second way
        // t_clear_scale(trf);
        // t_add_scale(trf, scale);
    }

    // void t_clear_scale(const Transform& trf)
    // {
    //     t_add_scale(trf, t_get_inverted_scale(trf));
    // }

    // void t_add_scale(const Transform& trf, const f32_3& scale)
    // {
    //     matrix._00 *= scale.x; matrix._01 *= scale.x; matrix._02 *= scale.x; // smdify?
    //     matrix._10 *= scale.y; matrix._11 *= scale.y; matrix._12 *= scale.y; // smdify?
    //     matrix._20 *= scale.z; matrix._21 *= scale.z; matrix._22 *= scale.z; // smdify?
    // }
}

#endif
