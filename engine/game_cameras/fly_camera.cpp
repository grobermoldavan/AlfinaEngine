#include "fly_camera.h"
#include "debug/debug.h"

namespace al::engine
{

    void construct(FlyCamera* flyCamera)
    {
        flyCamera->renderCamera = { };
        flyCamera->cursorPosition = { };
        flyCamera->cachedWheelState = 0;
        flyCamera->mouseSensitivity = 5.0f;
        flyCamera->isRmbPressed = false;
    }

    PerspectiveRenderCamera* get_render_camera(FlyCamera* flyCamera)
    {
        return &flyCamera->renderCamera;
    }

    void process_inputs(FlyCamera* flyCamera, const OsWindowInput* input, float dt)
    {
        al_profile_function();
        Transform* trf = flyCamera->renderCamera.get_transform();
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
                const bool isJustPressed = !flyCamera->isRmbPressed;
                const int32_t cursorX = input->mouse.x;
                const int32_t cursorY = input->mouse.y;
                if (!isJustPressed)
                {
                    const int32_2 diff{ cursorX - flyCamera->cursorPosition.x, cursorY - flyCamera->cursorPosition.y };
                    float3 rotation = trf->get_rotation();
                    rotation.x += static_cast<float>(diff.y) * dt * flyCamera->mouseSensitivity;
                    rotation.y += static_cast<float>(diff.x) * dt * flyCamera->mouseSensitivity;
                    trf->set_rotation(rotation);
                }
                flyCamera->cursorPosition.x = cursorX;
                flyCamera->cursorPosition.y = cursorY;
                flyCamera->isRmbPressed = true;
            }
            else
            {
                flyCamera->isRmbPressed = false;
            }
        }
        // Process mouse wheel input
        {
            const int32_t currentWheelState = input->mouse.wheel;
            const int32_t diff = currentWheelState - flyCamera->cachedWheelState;
            flyCamera->renderCamera.set_fov(flyCamera->renderCamera.get_fov() + (static_cast<float>(diff) * 0.005f));
            flyCamera->cachedWheelState = currentWheelState;
        }
    }
}
