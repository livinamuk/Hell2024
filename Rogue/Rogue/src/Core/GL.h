#pragma once
#include "../Common.h"

namespace GL {
	void Init(int width, int height);
	void ProcessInput();
	void SwapBuffersPollEvents();
	void Cleanup();
	bool WindowIsOpen();
	int GetWindowWidth();
	int GetWindowHeight();
	int GetCursorX();
	int GetCursorY();
	GLFWwindow* GetWindowPtr();
}