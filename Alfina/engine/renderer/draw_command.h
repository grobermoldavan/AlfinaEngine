#ifndef __ALFINA_DRAW_COMMAND_H__
#define __ALFINA_DRAW_COMMAND_H__

#include "command_buffer.h"

namespace al::engine
{
    void draw_command_dispatch_function(void* ptr);

    struct DrawCommand
    {
        static const cb::DispatchFunction dispatchFunction;
    };

    const cb::DispatchFunction DrawCommand::dispatchFunction = draw_command_dispatch_function;

    void draw_command_dispatch_function(void* ptr)
    {
        DrawCommand* command = reinterpret_cast<DrawCommand*>(ptr);

    }
}

#endif