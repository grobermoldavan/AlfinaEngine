#ifndef AL_FLY_CAMERA_H
#define AL_FLY_CAMERA_H

#include "engine/rendering/camera/perspective_render_camera.h"
#include "engine/window/os_window.h"

namespace al::engine
{
    class FlyCamera
    {
    public:
        FlyCamera() noexcept
            : cursorPosition{ }
            , cachedWheelState{ 0 }
            , mouseSensitivity{ 5.0f }
            , isRmbPressed{ false }
        { }

        ~FlyCamera() = default;

        PerspectiveRenderCamera* get_render_camera() noexcept
        {
            return &renderCamera;
        }

        void process_inputs(const OsWindowInput* input, float dt) noexcept
        {
            Transform* trf = renderCamera.get_transform();

            // Process keyboard input
            {
                constexpr float FAST_MOVEMENT_SPEED = 5.0f;
                constexpr float NORMAL_MOVEMENT_SPEED = 1.0f;
                const float movementSpeed = input->keyboard.buttons.get_flag(static_cast<uint32_t>(OsWindowInput::KeyboardInputFlags::CTRL)) ? FAST_MOVEMENT_SPEED : NORMAL_MOVEMENT_SPEED;

                float3 pos = trf->get_position();
                if (input->keyboard.buttons.get_flag(static_cast<uint32_t>(OsWindowInput::KeyboardInputFlags::W))) { pos += trf->get_forward()  * movementSpeed * dt; }
                if (input->keyboard.buttons.get_flag(static_cast<uint32_t>(OsWindowInput::KeyboardInputFlags::S))) { pos += trf->get_forward()  * movementSpeed * dt * -1.0f; }
                if (input->keyboard.buttons.get_flag(static_cast<uint32_t>(OsWindowInput::KeyboardInputFlags::A))) { pos += trf->get_right()    * movementSpeed * dt * -1.0f; }
                if (input->keyboard.buttons.get_flag(static_cast<uint32_t>(OsWindowInput::KeyboardInputFlags::D))) { pos += trf->get_right()    * movementSpeed * dt; }
                if (input->keyboard.buttons.get_flag(static_cast<uint32_t>(OsWindowInput::KeyboardInputFlags::Q))) { pos += trf->get_up()       * movementSpeed * dt * -1.0f; }
                if (input->keyboard.buttons.get_flag(static_cast<uint32_t>(OsWindowInput::KeyboardInputFlags::E))) { pos += trf->get_up()       * movementSpeed * dt; }
                trf->set_position(pos);
            }

            // Process mouse move input
            {
                if (input->mouse.buttons.get_flag(static_cast<uint32_t>(OsWindowInput::MouseInputFlags::RMB_PRESSED)))
                {
                    const bool isJustPressed = !isRmbPressed;
                    const int32_t cursorX = input->mouse.x;
                    const int32_t cursorY = input->mouse.y;

                    if (!isJustPressed)
                    {
                        const int32_2 diff{ cursorX - cursorPosition.x, cursorY - cursorPosition.y };
                        float3 rotation = trf->get_rotation();
                        rotation.x += static_cast<float>(diff.y) * dt * mouseSensitivity;
                        rotation.y += static_cast<float>(diff.x) * dt * mouseSensitivity;
                        trf->set_rotation(rotation);
                    }

                    cursorPosition.x = cursorX;
                    cursorPosition.y = cursorY;
                    isRmbPressed = true;
                }
                else
                {
                    isRmbPressed = false;
                }
            }

            // Process mouse wheel input
            {
                const int32_t currentWheelState = input->mouse.wheel;
                const int32_t diff = currentWheelState - cachedWheelState;
                renderCamera.set_fov(renderCamera.get_fov() + (static_cast<float>(diff) * 0.005f));
                cachedWheelState = currentWheelState;
            }
        }

    public:
        PerspectiveRenderCamera renderCamera;
        int32_2 cursorPosition;
        int32_t cachedWheelState;
        float mouseSensitivity;
        bool isRmbPressed;
    };
}

#endif
