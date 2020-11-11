#ifndef AL_VECTORS_H
#define AL_VECTORS_H

#include <cstring>

#include "../concepts.h"
#include "../constexpr_functions.h"

namespace al
{
    template<al::number Type>
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
        
        // @TODO : make a constexpr versions?
        inline void     set(const vec2& other)          noexcept    { std::memcpy(elements, other.elements, sizeof(Type) * 2); }
        inline Type     dot(const vec2& other)  const   noexcept    { return x * other.x + y * other.y; }
        inline vec2     add(const vec2& other)  const   noexcept    { return { x + other.x, y + other.y }; }
        inline vec2     sub(const vec2& other)  const   noexcept    { return { x - other.x, y - other.y }; }
        inline vec2     mul(Type value)         const   noexcept    { return { x * value, y * value }; }
        inline vec2     div(Type value)         const   noexcept    { return { x / value, y / value }; }
        inline float    len()                   const   noexcept    { return std::sqrt(x * x + y * y); }
        inline vec2     normalized()            const   noexcept    { float length = len(); return { x / length, y / length }; }
        inline vec2     normalize()                     noexcept    { set(normalized()); }

                inline vec2&    operator =  (const vec2& other)                     noexcept    { set(other); return *this; }
                inline vec2&    operator += (const vec2& other)                     noexcept    { set(add(other)); return *this; }
                inline vec2&    operator -= (const vec2& other)                     noexcept    { set(sub(other)); return *this; }
                inline vec2&    operator *= (Type other)                            noexcept    { set(mul(other)); return *this; }
                inline vec2&    operator /= (Type other)                            noexcept    { set(div(other)); return *this; }
        friend  inline bool     operator == (const vec2& one, const vec2& other)    noexcept    { return is_equal(one.x, other.x) && is_equal(one.y, other.y); }
        friend  inline bool     operator != (const vec2& one, const vec2& other)    noexcept    { return !is_equal(one.x, other.x) || !is_equal(one.y, other.y); }
        friend  inline Type     operator *  (const vec2& one, const vec2& other)    noexcept    { return one.dot(other); }
        friend  inline vec2     operator +  (const vec2& one, const vec2& other)    noexcept    { return one.add(other); }
        friend  inline vec2     operator -  (const vec2& one, const vec2& other)    noexcept    { return one.sub(other); }
        friend  inline vec2     operator *  (const vec2& one, Type value)           noexcept    { return one.mul(value); }
        friend  inline vec2     operator /  (const vec2& one, Type value)           noexcept    { return one.div(value); }
        friend  inline vec2     operator *  (Type value, const vec2& one)           noexcept    { return one.mul(value); }
        friend  inline vec2     operator /  (Type value, const vec2& one)           noexcept    { return one.div(value); }
    };

    template<al::number Type>
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

        // @TODO : make a constexpr versions?
        inline void     set(const vec3& other)          noexcept    { std::memcpy(elements, other.elements, sizeof(Type) * 3); }
        inline Type     dot(const vec3& other)  const   noexcept    { return x * other.x + y * other.y + z * other.z; }
        inline vec3     add(const vec3& other)  const   noexcept    { return { x + other.x, y + other.y, z + other.z }; }
        inline vec3     sub(const vec3& other)  const   noexcept    { return { x - other.x, y - other.y, z - other.z }; }
        inline vec3     mul(Type value)         const   noexcept    { return { x * value, y * value, z * value }; }
        inline vec3     div(Type value)         const   noexcept    { return { x / value, y / value, z / value }; }
        inline float    len()                   const   noexcept    { return std::sqrt(x * x + y * y + z * z); }
        inline vec3     normalized()            const   noexcept    { float length = len(); return { x / length, y / length, z / length }; }
        inline vec3     normalize()                     noexcept    { set(normalized()); }

        inline vec3 cross(const vec3& a) const noexcept { return { y * a.z - z * a.y, z * a.x - x * a.z, x * a.y - y * a.x }; }

                inline vec3&    operator =  (const vec3& other)                     noexcept    { set(other); return *this; }
                inline vec3&    operator += (const vec3& other)                     noexcept    { set(add(other)); return *this; }
                inline vec3&    operator -= (const vec3& other)                     noexcept    { set(sub(other)); return *this; }
                inline vec3&    operator *= (Type other)                            noexcept    { set(mul(other)); return *this; }
                inline vec3&    operator /= (Type other)                            noexcept    { set(div(other)); return *this; }
        friend  inline bool     operator == (const vec3& one, const vec3& other)    noexcept    { return is_equal(one.x, other.x) && is_equal(one.y, other.y) && is_equal(one.z, other.z); }
        friend  inline bool     operator != (const vec3& one, const vec3& other)    noexcept    { return !is_equal(one.x, other.x) || !is_equal(one.y, other.y) || !is_equal(one.z, other.z); }
        friend  inline Type     operator *  (const vec3& one, const vec3& other)    noexcept    { return one.dot(other); }
        friend  inline vec3     operator +  (const vec3& one, const vec3& other)    noexcept    { return one.add(other); }
        friend  inline vec3     operator -  (const vec3& one, const vec3& other)    noexcept    { return one.sub(other); }
        friend  inline vec3     operator *  (const vec3& one, Type value)           noexcept    { return one.mul(value); }
        friend  inline vec3     operator /  (const vec3& one, Type value)           noexcept    { return one.div(value); }
        friend  inline vec3     operator *  (Type value, const vec3& one)           noexcept    { return one.mul(value); }
        friend  inline vec3     operator /  (Type value, const vec3& one)           noexcept    { return one.div(value); }
    };

    template<al::number Type>
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

        // @TODO : make a constexpr versions?
        inline void     set(const vec4& other)          noexcept    { std::memcpy(elements, other.elements, sizeof(Type) * 3); }
        inline Type     dot(const vec4& other)  const   noexcept    { return x * other.x + y * other.y + z * other.z + w * other.w; }
        inline vec4     add(const vec4& other)  const   noexcept    { return { x + other.x, y + other.y, z + other.z, w + other.w }; }
        inline vec4     sub(const vec4& other)  const   noexcept    { return { x - other.x, y - other.y, z - other.z, w - other.w }; }
        inline vec4     mul(Type value)         const   noexcept    { return { x * value, y * value, z * value, w * value }; }
        inline vec4     div(Type value)         const   noexcept    { return { x / value, y / value, z / value, w / value }; }
        inline float    len()                   const   noexcept    { return std::sqrt(x * x + y * y + z * z + w * w); }
        inline vec4     normalized()            const   noexcept    { float length = len(); return { x / length, y / length, z / length, w / length }; }
        inline vec4     normalize()                     noexcept    { set(normalized()); }

        inline vec4 cross(const vec4& a) const noexcept { return { y * a.z - z * a.y, z * a.x - x * a.z, x * a.y - y * a.x }; }

                inline vec4&    operator =  (const vec4& other)                     noexcept    { set(other); return *this; }
                inline vec4&    operator += (const vec4& other)                     noexcept    { set(add(other)); return *this; }
                inline vec4&    operator -= (const vec4& other)                     noexcept    { set(sub(other)); return *this; }
                inline vec4&    operator *= (Type other)                            noexcept    { set(mul(other)); return *this; }
                inline vec4&    operator /= (Type other)                            noexcept    { set(div(other)); return *this; }
        friend  inline bool     operator == (const vec4& one, const vec4& other)    noexcept    { return is_equal(one.x, other.x) && is_equal(one.y, other.y) && is_equal(one.z, other.z) && is_equal(one.w, other.w); }
        friend  inline bool     operator != (const vec4& one, const vec4& other)    noexcept    { return !is_equal(one.x, other.x) || !is_equal(one.y, other.y) || !is_equal(one.z, other.z) || !is_equal(one.w, other.w); }
        friend  inline Type     operator *  (const vec4& one, const vec4& other)    noexcept    { return one.dot(other); }
        friend  inline vec4     operator +  (const vec4& one, const vec4& other)    noexcept    { return one.add(other); }
        friend  inline vec4     operator -  (const vec4& one, const vec4& other)    noexcept    { return one.sub(other); }
        friend  inline vec4     operator *  (const vec4& one, Type value)           noexcept    { return one.mul(value); }
        friend  inline vec4     operator /  (const vec4& one, Type value)           noexcept    { return one.div(value); }
        friend  inline vec4     operator *  (Type value, const vec4& one)           noexcept    { return one.mul(value); }
        friend  inline vec4     operator /  (Type value, const vec4& one)           noexcept    { return one.div(value); }
    };

    using float2 = vec2<float>;
    using float3 = vec3<float>;
    using float4 = vec4<float>;
}

#endif
