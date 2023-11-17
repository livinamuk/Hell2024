#pragma once
#include "keycodes.h"

namespace Input
{
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
	void PreventRightMouseHold();
	int GetMouseX();
	int GetMouseY();
}