#ifndef AL_FLY_CAMERA_H
#define AL_FLY_CAMERA_H

#include "engine/rendering/camera/perspective_render_camera.h"
#include "engine/window/os_window.h"

namespace al::engine
{
    struct FlyCamera
    {
        PerspectiveRenderCamera renderCamera;
        int32_2 cursorPosition;
        int32_t cachedWheelState;
        float mouseSensitivity;
        bool isRmbPressed;
    };

    void construct(FlyCamera* flyCamera);

    PerspectiveRenderCamera* get_render_camera(FlyCamera* flyCamera);
    void process_inputs(FlyCamera* flyCamera, const OsWindowInput* input, float dt);
}

#endif
