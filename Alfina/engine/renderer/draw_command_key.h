#ifndef __ALFINA_DRAW_COMMAND_KEY_H__
#define __ALFINA_DRAW_COMMAND_KEY_H__

#include <cstdint>

#include "command_buffer.h"
#include "engine/asserts/asserts.h"
#include "utilities/limited_integer.h"
#include "utilities/constexpr_functions.h"

namespace al::engine
{
    namespace draw_key
    {
        using Type = uint64_t;
        // Setup from http://alinloghin.com/articles/command_buffer.html
        // 3 bits - viewport
        // 3 bits - view layer (skybox, world, etc.)
        // 2 bits - translucency(blending) type (opaque, normal, additive subtractive)
        // 1 bit - command 
        // if opaque mode :
        //     23 bits - material
        //     24 bits - depth
        // if transparent mode :
        //     24 bits - depth
        //     23 bits - material
        // if command bit is 1 :
        //     7 bits - priority
        //     48 bits - unused

        // @NOTE : currently this setup is used
        // Setup from http://realtimecollisiondetection.net/blog/?p=86 (but a bit changed)
        // 2 bits - fullscreen layer (game layer, fullscreen effects layet, hud layer, etc.)
        // 3 bits - viewport (for splitscreen, mirrors, portals, etc.)
        // 3 bits - viewport layer (same as fullscreen layer, but for a given viewport - skybox layer, world layer, effects layer, hud layer, etc.)
        // 2 bits - translucency (blending) type (opaque, normal, additive subtractive)
        // 1 bit - command (used-defined custom command - clear depth buffer, switch render target, etc.)
        // if opaque
        //     21 bits - material
        //     32 bits - depth
        // if transparent
        //     32 bits - depth
        //     21 bits - material
        // if command bit set to 1
        //     53 bits - command info (command id, or function ptr maybe)
        // (material might have some bits for different passes)

        template<typename T>
        void handle_error(T given, T max)
        {
            // value is out of range
            al_assert(false);
        }

        void set_fullscreen_layer(uint64_t* value, luint8_t<MaxValueForBits<uint8_t, 2>, handle_error> layer)
        {
            *value = set_bits(*value, 64 - 2, 2, {layer.get()});
        }
        
        uint64_t get_fullscreen_layer(uint64_t value)
        {
            return value >> (64 - 2);
        }

        void set_viewport(uint64_t* value, luint8_t<MaxValueForBits<uint8_t, 3>, handle_error> viewport)
        {
            *value = set_bits(*value, 64 - 5, 3, {viewport.get()});
        }

        uint64_t get_viewport(uint64_t value)
        {
            return (value << 2) >> (64 - 3);
        }

        void set_viewport_layer(uint64_t* value, luint8_t<MaxValueForBits<uint8_t, 3>, handle_error> layer)
        {
            *value = set_bits(*value, 64 - 8, 3, {layer.get()});
        }

        uint64_t get_viewport_layer(uint64_t value)
        {
            return (value << 5) >> (64 - 3);
        }

        void set_blending_type(uint64_t* value, luint8_t<MaxValueForBits<uint8_t, 2>, handle_error> type)
        {
            *value = set_bits(*value, 64 - 10, 2, {type.get()});
        }

        uint64_t get_blending_type(uint64_t value)
        {
            return (value << 8) >> (64 - 2);
        }

        void set_command_bit(uint64_t* value, luint8_t<MaxValueForBits<uint8_t, 1>, handle_error> type)
        {
            *value = set_bits(*value, 64 - 11, 1, {type.get()});
        }

        uint64_t get_command_bit(uint64_t value)
        {
            return (value << 10) >> (64 - 1);
        }

        void set_command(uint64_t* value, luint64_t<MaxValueForBits<uint64_t, 53>, handle_error> type)
        {
            *value = set_bits(*value, 0, 53, {type.get()});
        }

        uint64_t get_command(uint64_t value)
        {
            return (value << 11) >> 11;
        }

        void set_depth(uint64_t* value, luint32_t<MaxValueForBits<uint32_t, 32>, handle_error> type)
        {
            uint64_t blendingType = get_blending_type(*value);
            if (blendingType == 0)
            {
                *value = set_bits(*value, 0, 32, {type.get()});
            }
            else
            {
                *value = set_bits(*value, 21, 32, {type.get()});
            }
        }

        uint64_t get_depth(uint64_t value)
        {
            uint64_t blendingType = get_blending_type(value);
            if (blendingType == 0)
            {
                return (value << (64 - 32)) >> (64 - 32);
            }
            else
            {
                return (value << (64 - 53)) >> (64 - 32);
            }
        }

        void set_material(uint64_t* value, luint32_t<MaxValueForBits<uint32_t, 21>, handle_error> type)
        {
            auto blendingType = get_blending_type(*value);
            if (blendingType == 0)
            {
                *value = set_bits(*value, 32, 21, {type.get()});
            }
            else
            {
                *value = set_bits(*value, 0, 21, {type.get()});
            }
        }

        uint64_t get_material(uint64_t value)
        {
            uint64_t blendingType = get_blending_type(value);
            if (blendingType == 0)
            {
                return (value << (64 - 53)) >> (64 - 21);
            }
            else
            {
                return (value << (64 - 21)) >> (64 - 21);
            }
        }
    }
}

#endif
