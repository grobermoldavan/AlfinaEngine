#ifndef AL_MATRICES_H
#define AL_MATRICES_H

#include <iostream>

#include "utilities/concepts.h"
#include "utilities/constexpr_functions.h"
#include "vectors.h"

namespace al
{
    template<al::number Type>
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

        void set(const mat4x4& other) noexcept
        {
            std::memcpy(elements, other.elements, sizeof(Type) * 16);
        }
        
        mat4x4 mul(const mat4x4& other) const noexcept
        {
            return
            {
                (other.m[0][0] * m[0][0]) + (other.m[1][0] * m[0][1]) + (other.m[2][0] * m[0][2]) + (other.m[3][0] * m[0][3]),  // 0 0
                (other.m[0][1] * m[0][0]) + (other.m[1][1] * m[0][1]) + (other.m[2][1] * m[0][2]) + (other.m[3][1] * m[0][3]),  // 0 1
                (other.m[0][2] * m[0][0]) + (other.m[1][2] * m[0][1]) + (other.m[2][2] * m[0][2]) + (other.m[3][2] * m[0][3]),  // 0 2
                (other.m[0][3] * m[0][0]) + (other.m[1][3] * m[0][1]) + (other.m[2][3] * m[0][2]) + (other.m[3][3] * m[0][3]),  // 0 3
                (other.m[0][0] * m[1][0]) + (other.m[1][0] * m[1][1]) + (other.m[2][0] * m[1][2]) + (other.m[3][0] * m[1][3]),  // 1 0
                (other.m[0][1] * m[1][0]) + (other.m[1][1] * m[1][1]) + (other.m[2][1] * m[1][2]) + (other.m[3][1] * m[1][3]),  // 1 1
                (other.m[0][2] * m[1][0]) + (other.m[1][2] * m[1][1]) + (other.m[2][2] * m[1][2]) + (other.m[3][2] * m[1][3]),  // 1 2
                (other.m[0][3] * m[1][0]) + (other.m[1][3] * m[1][1]) + (other.m[2][3] * m[1][2]) + (other.m[3][3] * m[1][3]),  // 1 3
                (other.m[0][0] * m[2][0]) + (other.m[1][0] * m[2][1]) + (other.m[2][0] * m[2][2]) + (other.m[3][0] * m[2][3]),  // 2 0
                (other.m[0][1] * m[2][0]) + (other.m[1][1] * m[2][1]) + (other.m[2][1] * m[2][2]) + (other.m[3][1] * m[2][3]),  // 2 1
                (other.m[0][2] * m[2][0]) + (other.m[1][2] * m[2][1]) + (other.m[2][2] * m[2][2]) + (other.m[3][2] * m[2][3]),  // 2 2
                (other.m[0][3] * m[2][0]) + (other.m[1][3] * m[2][1]) + (other.m[2][3] * m[2][2]) + (other.m[3][3] * m[2][3]),  // 2 3
                (other.m[0][0] * m[3][0]) + (other.m[1][0] * m[3][1]) + (other.m[2][0] * m[3][2]) + (other.m[3][0] * m[3][3]),  // 3 0
                (other.m[0][1] * m[3][0]) + (other.m[1][1] * m[3][1]) + (other.m[2][1] * m[3][2]) + (other.m[3][1] * m[3][3]),  // 3 1
                (other.m[0][2] * m[3][0]) + (other.m[1][2] * m[3][1]) + (other.m[2][2] * m[3][2]) + (other.m[3][2] * m[3][3]),  // 3 2
                (other.m[0][3] * m[3][0]) + (other.m[1][3] * m[3][1]) + (other.m[2][3] * m[3][2]) + (other.m[3][3] * m[3][3])   // 3 3
            };
        }

        mat4x4 mul(Type value) const noexcept
        {
            return
            {
                m[0][0] * value, m[0][1] * value, m[0][2] * value, m[0][3] * value,
                m[1][0] * value, m[1][1] * value, m[1][2] * value, m[1][3] * value,
                m[2][0] * value, m[2][1] * value, m[2][2] * value, m[2][3] * value,
                m[3][0] * value, m[3][1] * value, m[3][2] * value, m[3][3] * value
            };
        }

        vec4<Type> mul(const vec4<Type>& value) const noexcept
        {
            return
            {
                value.x * m[0][0] + value.y * m[0][1] + value.z * m[0][2] + value.w * m[0][3],
                value.x * m[1][0] + value.y * m[1][1] + value.z * m[1][2] + value.w * m[1][3],
                value.x * m[2][0] + value.y * m[2][1] + value.z * m[2][2] + value.w * m[2][3],
                value.x * m[3][0] + value.y * m[3][1] + value.z * m[3][2] + value.w * m[3][3]
            };
        }

        mat4x4 transposed() const noexcept
        {
            return
            {
                _00, _10, _20, _30,
                _01, _11, _21, _31,
                _02, _12, _22, _32,
                _03, _13, _23, _33
            };
        }

        Type det() const noexcept
        {
            // 2x2 sub-determinants
            const float det2_01_01 = m[0][0] * m[1][1] - m[0][1] * m[1][0];
            const float det2_01_02 = m[0][0] * m[1][2] - m[0][2] * m[1][0];
            const float det2_01_03 = m[0][0] * m[1][3] - m[0][3] * m[1][0];
            const float det2_01_12 = m[0][1] * m[1][2] - m[0][2] * m[1][1];
            const float det2_01_13 = m[0][1] * m[1][3] - m[0][3] * m[1][1];
            const float det2_01_23 = m[0][2] * m[1][3] - m[0][3] * m[1][2];

            // 3x3 sub-determinants
            const float det3_201_012 = m[2][0] * det2_01_12 - m[2][1] * det2_01_02 + m[2][2] * det2_01_01;
            const float det3_201_013 = m[2][0] * det2_01_13 - m[2][1] * det2_01_03 + m[2][3] * det2_01_01;
            const float det3_201_023 = m[2][0] * det2_01_23 - m[2][2] * det2_01_03 + m[2][3] * det2_01_02;
            const float det3_201_123 = m[2][1] * det2_01_23 - m[2][2] * det2_01_13 + m[2][3] * det2_01_12;

            return Type{ -1 } * det3_201_123 * m[3][0] + det3_201_023 * m[3][1] - det3_201_013 * m[3][2] + det3_201_012 * m[3][3];
        }

        mat4x4 inverted() const noexcept
        {
            // https://stackoverflow.com/questions/1148309/inverting-a-4x4-matrix
            // https://github.com/willnode/N-Matrix-Programmer

            const Type determinant = det();
            if (is_equal(determinant, Type{ 0 }))
            {
                // @TODO : add method which returns bool here
                // Can't invert
                return { };
            }
            const Type invDet = Type{ 1.0 } / determinant;

            const Type A2323 = m[2][2] * m[3][3] - m[2][3] * m[3][2];
            const Type A1323 = m[2][1] * m[3][3] - m[2][3] * m[3][1];
            const Type A1223 = m[2][1] * m[3][2] - m[2][2] * m[3][1];
            const Type A0323 = m[2][0] * m[3][3] - m[2][3] * m[3][0];
            const Type A0223 = m[2][0] * m[3][2] - m[2][2] * m[3][0];
            const Type A0123 = m[2][0] * m[3][1] - m[2][1] * m[3][0];
            const Type A2313 = m[1][2] * m[3][3] - m[1][3] * m[3][2];
            const Type A1313 = m[1][1] * m[3][3] - m[1][3] * m[3][1];
            const Type A1213 = m[1][1] * m[3][2] - m[1][2] * m[3][1];
            const Type A2312 = m[1][2] * m[2][3] - m[1][3] * m[2][2];
            const Type A1312 = m[1][1] * m[2][3] - m[1][3] * m[2][1];
            const Type A1212 = m[1][1] * m[2][2] - m[1][2] * m[2][1];
            const Type A0313 = m[1][0] * m[3][3] - m[1][3] * m[3][0];
            const Type A0213 = m[1][0] * m[3][2] - m[1][2] * m[3][0];
            const Type A0312 = m[1][0] * m[2][3] - m[1][3] * m[2][0];
            const Type A0212 = m[1][0] * m[2][2] - m[1][2] * m[2][0];
            const Type A0113 = m[1][0] * m[3][1] - m[1][1] * m[3][0];
            const Type A0112 = m[1][0] * m[2][1] - m[1][1] * m[2][0];

            return
            {
                invDet              * (m[1][1] * A2323 - m[1][2] * A1323 + m[1][3] * A1223),    // 0 0
                invDet * Type{ -1 } * (m[0][1] * A2323 - m[0][2] * A1323 + m[0][3] * A1223),    // 0 1
                invDet              * (m[0][1] * A2313 - m[0][2] * A1313 + m[0][3] * A1213),    // 0 2
                invDet * Type{ -1 } * (m[0][1] * A2312 - m[0][2] * A1312 + m[0][3] * A1212),    // 0 3
                invDet * Type{ -1 } * (m[1][0] * A2323 - m[1][2] * A0323 + m[1][3] * A0223),    // 1 0
                invDet              * (m[0][0] * A2323 - m[0][2] * A0323 + m[0][3] * A0223),    // 1 1
                invDet * Type{ -1 } * (m[0][0] * A2313 - m[0][2] * A0313 + m[0][3] * A0213),    // 1 2
                invDet              * (m[0][0] * A2312 - m[0][2] * A0312 + m[0][3] * A0212),    // 1 3
                invDet              * (m[1][0] * A1323 - m[1][1] * A0323 + m[1][3] * A0123),    // 2 0
                invDet * Type{ -1 } * (m[0][0] * A1323 - m[0][1] * A0323 + m[0][3] * A0123),    // 2 1
                invDet              * (m[0][0] * A1313 - m[0][1] * A0313 + m[0][3] * A0113),    // 2 2
                invDet * Type{ -1 } * (m[0][0] * A1312 - m[0][1] * A0312 + m[0][3] * A0112),    // 2 3
                invDet * Type{ -1 } * (m[1][0] * A1223 - m[1][1] * A0223 + m[1][2] * A0123),    // 3 0
                invDet              * (m[0][0] * A1223 - m[0][1] * A0223 + m[0][2] * A0123),    // 3 1
                invDet * Type{ -1 } * (m[0][0] * A1213 - m[0][1] * A0213 + m[0][2] * A0113),    // 3 2
                invDet              * (m[0][0] * A1212 - m[0][1] * A0212 + m[0][2] * A0112)     // 3 3
            };
        }

                inline mat4x4       operator = (const mat4x4& other)                        noexcept { set(other); return *this; }
        friend  inline mat4x4       operator * (const mat4x4& one, const mat4x4& other)     noexcept { return one.mul(other); }
        friend  inline mat4x4       operator * (const mat4x4& mat, Type value)              noexcept { return mat.mul(value); }
        friend  inline mat4x4       operator * (Type value, const mat4x4& mat)              noexcept { return mat.mul(value); }
        friend  inline vec4<Type>   operator * (const mat4x4& mat, const vec4<Type>& vec)   noexcept { return mat.mul(vec); }

        friend std::ostream& operator << (std::ostream& os, const mat4x4& mat) noexcept
        {
            os << "[" << mat.m[0][0] << ", " << mat.m[0][1] << ", " << mat.m[0][2] << ", " << mat.m[0][3] << "]" << std::endl;
            os << "[" << mat.m[1][0] << ", " << mat.m[1][1] << ", " << mat.m[1][2] << ", " << mat.m[1][3] << "]" << std::endl;
            os << "[" << mat.m[2][0] << ", " << mat.m[2][1] << ", " << mat.m[2][2] << ", " << mat.m[2][3] << "]" << std::endl;
            os << "[" << mat.m[3][0] << ", " << mat.m[3][1] << ", " << mat.m[3][2] << ", " << mat.m[3][3] << "]" << std::endl;
            return os;
        }
    };
    
    using float4x4 = mat4x4<float>;

    constexpr float4x4 IDENTITY4
    {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
}

#endif
