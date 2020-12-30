#ifndef AL_FLY_CAMERA_H
#define AL_FLY_CAMERA_H

#include "engine/rendering/camera/perspective_render_camera.h"
#include "engine/window/os_window.h"

namespace al::engine
{
    class FlyCamera
    {
    public:
        FlyCamera() noexcept;
        ~FlyCamera() = default;

        PerspectiveRenderCamera* get_render_camera() noexcept;
        void process_inputs(const OsWindowInput* input, float dt) noexcept;

    public:
        PerspectiveRenderCamera renderCamera;
        int32_2 cursorPosition;
        int32_t cachedWheelState;
        float mouseSensitivity;
        bool isRmbPressed;
    };
}

#endif
