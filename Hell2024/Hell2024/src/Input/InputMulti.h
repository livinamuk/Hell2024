#pragma once

#include "keycodes.h"
#include <vector>

struct MouseState {
    bool detected = false;
    bool leftMouseDown = false;
    bool rightMouseDown = false;
    bool leftMousePressed = false;
    bool rightMousePressed = false;
    bool leftMouseDownLastFrame = false;
    bool rightMouseDownLastFrame = false;
    int xoffset = 0;
    int yoffset = 0;
};

struct KeyboardState {
    bool keyPressed[372];
    bool keyDown[372];
    bool keyDownLastFrame[372];
};

namespace InputMulti {

    void Init();
    void Update();

    bool LeftMouseDown(int index);
    bool RightMouseDown(int index);
    bool LeftMousePressed(int index);
    bool RightMousePressed(int index);
    int GetMouseXOffset(int index);
    int GetMouseYOffset(int index);
    bool KeyPressed(int keyboardIndex, int mouseIndex, unsigned int keycode);
    bool KeyDown(int keyboardIndex, int mouseIndex, unsigned int keycode);
    void ResetMouseOffsets();
}