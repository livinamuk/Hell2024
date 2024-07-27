#pragma once
#include "keycodes.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace Input {

	void Init();
	void Update();
	bool KeyPressed(unsigned int keycode);
	bool KeyDown(unsigned int keycode);
	float GetMouseOffsetX();
	float GetMouseOffsetY();
	bool LeftMouseDown();
	bool RightMouseDown();
	bool LeftMousePressed();
	bool RightMousePressed();
	bool MouseWheelUp();
	bool MouseWheelDown();
	int GetMouseWheelValue();
	int GetMouseX();
	int GetMouseY();
	void PreventRightMouseHold();
    int GetScrollWheelYOffset();
    void ResetScrollWheelYOffset();
    //int GetCursorX();
    //int GetCursorY();
    void DisableCursor();
    void HideCursor();
    void ShowCursor();
    int GetCursorScreenX();
    int GetCursorScreenY();
    int GetViewportMappedMouseX(int viewportWidth);
    int GetViewportMappedMouseY(int viewportHeight);
}