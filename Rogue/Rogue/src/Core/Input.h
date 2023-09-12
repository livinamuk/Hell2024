#pragma once
#include <GLFW/glfw3.h>
#include "keycodes.h"

namespace Input
{
	void Init(GLFWwindow* window);
	void Update(GLFWwindow* window);
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
	void PreventRightMouseHold();
	int GetMouseX();
	int GetMouseY();
}