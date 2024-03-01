#include "InputMulti.h"
#include "GL.h"
#include <iostream>

#define NOMINMAX
#include <Windows.h>
#ifdef _MSC_VER
#undef GetObject
#endif

std::vector<MouseState> _mouseStates;
std::vector<KeyboardState> _keyboardStates;

const USHORT HID_USAGE_GENERIC_MOUSE = 0x02;
const USHORT HID_USAGE_GENERIC_KEYBOARD = 0x06;

std::vector<HANDLE> mouseHandles;
std::vector<HANDLE> keyboardHandles;

int GetHandleIndex(std::vector<HANDLE>* handleVector, HANDLE handle) {
    for (int i = 0; i < handleVector->size(); i++) {
        if ((*handleVector)[i] == handle) {
            return i;
        }
    }
    handleVector->push_back(handle);
    return (int)handleVector->size() - 1;
}

LRESULT CALLBACK targetWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_INPUT) {
        UINT dataSize = 0;
        // First call to get data size
        GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, NULL, &dataSize, sizeof(RAWINPUTHEADER));

        if (dataSize > 0)
        {
            RAWINPUT raw = RAWINPUT();
            // Second call to get the actual data
            if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, &raw, &dataSize, sizeof(RAWINPUTHEADER)) == dataSize)
            {
                // Mice
                if (raw.header.dwType == RIM_TYPEMOUSE)
                {
                    int mouseID = GetHandleIndex(&mouseHandles, raw.header.hDevice);
                    if (mouseID >= 4) return 0;

                    switch (raw.data.mouse.ulButtons) {
                    case RI_MOUSE_LEFT_BUTTON_DOWN: _mouseStates[mouseID].leftMouseDown = true;	break;
                    case RI_MOUSE_LEFT_BUTTON_UP: _mouseStates[mouseID].leftMouseDown = false; break;
                    case RI_MOUSE_RIGHT_BUTTON_DOWN: _mouseStates[mouseID].rightMouseDown = true; break;
                    case RI_MOUSE_RIGHT_BUTTON_UP: _mouseStates[mouseID].rightMouseDown = false; break;
                    }

                    // Wheel change values are device-dependent. Check RAWMOUSE docs for details.
                    if (raw.data.mouse.usButtonData != 0) {
                        //	cout << "MOUSE " << mouseID << ": WHEEL CHANGE " << raw.data.mouse.usButtonData << endl;
                    }

                    _mouseStates[mouseID].xoffset += raw.data.mouse.lLastX;
                    _mouseStates[mouseID].yoffset += raw.data.mouse.lLastY;
                }
                // Keyboard
                else if (raw.header.dwType == RIM_TYPEKEYBOARD)
                {
                    int keyboardID = GetHandleIndex(&keyboardHandles, raw.header.hDevice);
                    auto keycode = raw.data.keyboard.VKey;
                    if (keyboardID >= 4) return 0;

                    //std::cout << keycode << "\n";

                    switch (raw.data.keyboard.Flags) {
                    case RI_KEY_MAKE: _keyboardStates[keyboardID].keyDown[keycode] = true; break;
                    case RI_KEY_BREAK: _keyboardStates[keyboardID].keyDown[keycode] = false; break;
                    }
                }
            }
        }
        return 0;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

bool RegisterDeviceOfType(USHORT type, HWND eventWindow) {
    RAWINPUTDEVICE rid = {};
    rid.usUsagePage = 0x01;
    rid.usUsage = type;
    rid.dwFlags = RIDEV_INPUTSINK;
    rid.hwndTarget = eventWindow;
    return RegisterRawInputDevices(&rid, 1, sizeof(rid));
}

void InputMulti::Init() {

    HINSTANCE hInstance = GetModuleHandle(NULL);
    WNDCLASS windowClass = {};
    windowClass.lpfnWndProc = targetWindowProc;
    windowClass.hInstance = hInstance;
    windowClass.lpszClassName = TEXT("InputWindow");
    if (!RegisterClass(&windowClass)) {
        std::cout << "Failed to register window class\n";
        return;
    }

    HWND eventWindow = CreateWindowEx(0, windowClass.lpszClassName, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);
    if (!eventWindow) {
        std::cout << "Failed to register window class\n";
        return;
    }
    else
        std::cout << "Dual keyboard init successful\n";
    RegisterDeviceOfType(HID_USAGE_GENERIC_MOUSE, eventWindow);
    RegisterDeviceOfType(HID_USAGE_GENERIC_KEYBOARD, eventWindow);

    // Add support for 2 mice/keyboard
    for (int i = 0; i < 2; i++) {
        _mouseStates.push_back(MouseState());
        _keyboardStates.push_back(KeyboardState());
    }
}

#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#define GLFW_NATIVE_INCLUDE_NONE
#include <GLFW/glfw3native.h>

void InputMulti::Update()
{
    MSG msg;
    //	while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    for (MouseState& state : _mouseStates) {
        // Left mouse down/pressed
        if (state.leftMouseDown && !state.leftMouseDownLastFrame)
            state.leftMousePressed = true;
        else
            state.leftMousePressed = false;
        state.leftMouseDownLastFrame = state.leftMouseDown;

        // Right mouse down/pressed
        if (state.rightMouseDown && !state.rightMouseDownLastFrame)
            state.rightMousePressed = true;
        else
            state.rightMousePressed = false;
        state.rightMouseDownLastFrame = state.rightMouseDown;
    }

    for (KeyboardState& state : _keyboardStates) {
        // Key press
        for (int i = 0; i < 350; i++) {
            if (state.keyDown[i] && !state.keyDownLastFrame[i])
                state.keyPressed[i] = true;
            else
                state.keyPressed[i] = false;
            state.keyDownLastFrame[i] = state.keyDown[i];
        }
    }

    // Out of window focus? then remove any detected input
    HWND activeWindow = GetActiveWindow();
    HWND myWindow = glfwGetWin32Window(GL::GetWindowPtr());
    if ((void*)myWindow != (void*)activeWindow) {
        for (KeyboardState& state : _keyboardStates) {
            for (int i = 0; i < 350; i++) {
                state.keyPressed[i] = false;
                state.keyDownLastFrame[i] = false;
            }
        }
        for (MouseState& state : _mouseStates) {
            state.leftMousePressed = false;
            state.leftMouseDownLastFrame = false;
            state.rightMousePressed = false;
            state.rightMouseDownLastFrame = false;
        }
    }
}

void InputMulti::ResetMouseOffsets() {
    for (MouseState& state : _mouseStates) {
        state.xoffset = 0;
        state.yoffset = 0;
    }
}

int InputMulti::GetMouseYOffset(int index) {
    if (index < 0 || index >= 4)
        return 0;
    else
        return _mouseStates[index].yoffset;
}

bool InputMulti::LeftMouseDown(int index) {
    if (index < 0 || index >= 4)
        return false;
    else
        return _mouseStates[index].leftMouseDown;
}

bool InputMulti::RightMouseDown(int index) {
    if (index < 0 || index >= 4)
        return false;
    else
        return _mouseStates[index].rightMouseDown;
}

bool InputMulti::LeftMousePressed(int index) {
    if (index < 0 || index >= 4)
        return false;
    else
        return _mouseStates[index].leftMousePressed;
}

bool InputMulti::RightMousePressed(int index) {
    if (index < 0 || index >= 4)
        return false;
    else
        return _mouseStates[index].rightMousePressed;
}

int InputMulti::GetMouseXOffset(int index) {
    if (index < 0 || index >= 4)
        return 0;
    else
        return _mouseStates[index].xoffset;
}

bool InputMulti::KeyDown(int keyboardIndex, int mouseIndex, unsigned int keycode) {
    // It's a mouse button
    if (keycode == HELL_MOUSE_LEFT && mouseIndex >= 0 && mouseIndex < 4) {
        return _mouseStates[mouseIndex].leftMouseDown;
    }
    else if (keycode == HELL_MOUSE_RIGHT && mouseIndex >= 0 && mouseIndex < 4) {
        return _mouseStates[mouseIndex].rightMouseDown;
    }

    // It's a keyboard button
    else if (keyboardIndex >= 0 && keyboardIndex < 4) {
        return _keyboardStates[keyboardIndex].keyDown[keycode];
    }
    // Something else invalid
    else {
        return false;
    }
}

bool InputMulti::KeyPressed(int keyboardIndex, int mouseIndex, unsigned int keycode) {

    // It's a mouse button
    if (keycode == HELL_MOUSE_LEFT && mouseIndex >= 0 && mouseIndex < 4)
        return _mouseStates[mouseIndex].leftMousePressed;
    else if (keycode == HELL_MOUSE_RIGHT && mouseIndex >= 0 && mouseIndex < 4)
        return _mouseStates[mouseIndex].rightMousePressed;

    // It's a keyboard button
    else if (keyboardIndex >= 0 && keyboardIndex < 4)
        return _keyboardStates[keyboardIndex].keyPressed[keycode];
    // Something else invalid
    else
        return false;
}

