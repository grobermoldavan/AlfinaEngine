#ifndef AL_PLATFORM_INPUT_H
#define AL_PLATFORM_INPUT_H

namespace al
{
    enum class MouseInput
    {
        LMB,
        RMB,
        MMB,
        __end
    };

    enum class KeyboardInput
    {
        NONE,
        Q, W, E, R, T, Y, U, I, O, P, A, S, D, F, G, H, J, K, L, Z, X, C, V, B, N, M,
        L_BRACKET, R_BRACKET, SEMICOLON, APOSTROPHE, BACKSLASH, COMMA, PERIOD, SLASH,
        TILDA, NUM_1, NUM_2, NUM_3, NUM_4, NUM_5, NUM_6, NUM_7, NUM_8, NUM_9, NUM_0, MINUS, EQUALS,
        TAB, CAPS_LOCK, ENTER, BACKSPACE, SPACE,
        L_SHIFT, R_SHIFT, SHIFT,
        L_CTRL, R_CTRL, CTRL,
        L_ALT, R_ALT, ALT,
        ESCAPE, F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
        ARROW_UP, ARROW_DOWN, ARROW_LEFT, ARROW_RIGHT, __end
    };

    const char* KEYBOARD_INPUT_TO_STRING[] =
    {
        "NONE",
        "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "A", "S", "D", "F", "G", "H", "J", "K", "L", "Z", "X", "C", "V", "B", "N", "M",
        "L_BRACKET", "R_BRACKET", "SEMICOLON", "APOSTROPHE", "BACKSLASH", "COMMA", "PERIOD", "SLASH",
        "TILDA", "NUM_1", "NUM_2", "NUM_3", "NUM_4", "NUM_5", "NUM_6", "NUM_7", "NUM_8", "NUM_9", "NUM_0", "MINUS", "EQUALS",
        "TAB", "CAPS_LOCK", "ENTER", "BACKSPACE", "SPACE",
        "L_SHIFT", "R_SHIFT", "SHIFT",
        "L_CTRL", "R_CTRL", "CTRL",
        "L_ALT", "R_ALT", "ALT",
        "ESCAPE", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12",
        "ARROW_UP", "ARROW_DOWN", "ARROW_LEFT", "ARROW_RIGHT"
    };

    const char* MOUSE_INPUT_TO_STRING[] =
    {
        "LMB", "RMB", "MMB"
    };

    struct PlatformInput;

    void platform_input_construct(PlatformInput* input);
    void platform_input_destruct(PlatformInput* input);
    void platform_input_update(PlatformInput* input);

    bool platform_input_is_mouse_input_active(PlatformInput* input, MouseInput flag);
    bool platform_input_is_keyboard_input_active(PlatformInput* input, KeyboardInput flag);
}

#endif
