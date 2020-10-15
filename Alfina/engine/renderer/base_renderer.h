#ifndef __ALFINA_BASE_RENDERER_H__
#define __ALFINA_BASE_RENDERER_H__

#include "engine/engine_utilities/error_info.h"
#include "engine/memory/stack_allocator.h"

#include "base_shader.h"
#include "base_vertex_buffer.h"
#include "base_index_buffer.h"
#include "base_vertex_array.h"

#include "command_buffer.h"
#include "draw_command_key.h"
#include "draw_command.h"

#include "utilities/limited_integer.h"

namespace al::engine
{
    enum BlendingMode
    {
        MODE_OPAQUE,
        MODE_NORMAL,
        MODE_ADDITIVE,
        MODE_SUBTRACTIVE
    };

	class Renderer
	{
	public:
        Renderer(ApplicationWindow* window, StackAllocator* allocator);
		virtual ~Renderer() = default;

		virtual void commit	() = 0;
		virtual void wait	() = 0;

    protected:
        CommandBuffer<draw_key::Type> commandBuffer;
        StackAllocator* allocator;
	};

    void handle(uint32_t max, uint32_t val)
    {
        al_assert(false);
    }

    Renderer::Renderer(ApplicationWindow* window, StackAllocator* allocator)
        : allocator{allocator}
    { 
        //uint64_t value = 0;
        //draw_key::set_fullscreen_layer(&value, 1);
        //draw_key::set_viewport(&value, 1);
        //draw_key::set_viewport_layer(&value, 1);
        //draw_key::set_blending_type(&value, BlendingMode::MODE_OPAQUE);
        //draw_key::set_command_bit(&value, 1);
        //draw_key::set_depth(&value, 1);
        //draw_key::set_material(&value, 1);

        //DrawCommand* command = commandBuffer.add_command<DrawCommand>(value, 0);
    }
}

#endif
