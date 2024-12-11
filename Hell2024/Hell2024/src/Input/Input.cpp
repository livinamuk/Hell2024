#include "Input.h"
#include "../BackEnd/BackEnd.h"
#include "../Renderer/Renderer.h"
#include "../Util.hpp"

namespace Input {

    bool _keyPressed[372];
    bool _keyDown[372];
    bool _keyDownLastFrame[372];
    double _mouseX = 0;
    double _mouseY = 0;
    double _mouseOffsetX = 0;
    double _mouseOffsetY = 0;
    int _mouseWheelValue = 0;
    int _sensitivity = 100;
    bool _mouseWheelUp = false;
    bool _mouseWheelDown = false;
    bool _leftMouseDown = false;
    bool _rightMouseDown = false;
    bool _leftMousePressed = false;
    bool _rightMousePressed = false;
    bool _leftMouseDownLastFrame = false;
    bool _rightMouseDownLastFrame = false;
    bool _preventRightMouseHoldTillNextClick = false;
    int _mouseScreenX = 0;
    int _mouseScreenY = 0;
    int _scrollWheelYOffset = 0;
    GLFWwindow* _window;

    void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

    void Init() {

        double x, y;
        _window = BackEnd::GetWindowPointer();
        glfwSetScrollCallback(_window, scroll_callback);
        glfwGetCursorPos(_window, &x, &y);
        DisableCursor();
        _mouseOffsetX = x;
        _mouseOffsetY = y;
        _mouseX = x;
        _mouseY = y;
    }

    void Update() {

        if (KeyPressed(HELL_KEY_ESCAPE)) {
            BackEnd::ForceCloseWindow();
        }
        if (KeyPressed(HELL_KEY_G)) {
           BackEnd::ToggleFullscreen();
        }
        if (KeyPressed(HELL_KEY_H)) {
            Renderer::HotloadShaders();
        }

        // Wheel
        _mouseWheelUp = false;
        _mouseWheelDown = false;
        _mouseWheelValue = GetScrollWheelYOffset();
        if (_mouseWheelValue < 0)
            _mouseWheelDown = true;
        if (_mouseWheelValue > 0)
            _mouseWheelUp = true;
        ResetScrollWheelYOffset();

        // Keyboard
        for (int i = 32; i < 349; i++) {
            // down
            if (glfwGetKey(_window, i) == GLFW_PRESS)
                _keyDown[i] = true;
            else
                _keyDown[i] = false;

            // press
            if (_keyDown[i] && !_keyDownLastFrame[i])
                _keyPressed[i] = true;
            else
                _keyPressed[i] = false;
            _keyDownLastFrame[i] = _keyDown[i];
        }

        // Mouse
        double x, y;
        glfwGetCursorPos(_window, &x, &y);
        _mouseOffsetX = x - _mouseX;
        _mouseOffsetY = y - _mouseY;
        _mouseX = x;
        _mouseY = y;

        // Left mouse down/pressed
        _leftMouseDown = glfwGetMouseButton(_window, GLFW_MOUSE_BUTTON_LEFT);
        if (_leftMouseDown == GLFW_PRESS && !_leftMouseDownLastFrame)
            _leftMousePressed = true;
        else
            _leftMousePressed = false;
        _leftMouseDownLastFrame = _leftMouseDown;

        // Right mouse down/pressed
        _rightMouseDown = glfwGetMouseButton(_window, GLFW_MOUSE_BUTTON_RIGHT);
        if (_rightMouseDown == GLFW_PRESS && !_rightMouseDownLastFrame)
            _rightMousePressed = true;
        else
            _rightMousePressed = false;
        _rightMouseDownLastFrame = _rightMouseDown;

        if (_rightMousePressed)
            _preventRightMouseHoldTillNextClick = false;
    }

    bool KeyPressed(unsigned int keycode) {
        return _keyPressed[keycode];
    }

    bool KeyDown(unsigned int keycode) {
        return _keyDown[keycode];
    }

    float GetMouseOffsetX() {
        return (float)_mouseOffsetX;
    }

    float GetMouseOffsetY() {
        return (float)_mouseOffsetY;
    }

    bool LeftMouseDown() {
        return _leftMouseDown;
    }

    bool RightMouseDown() {
        return _rightMouseDown && !_preventRightMouseHoldTillNextClick;
    }

    bool LeftMousePressed() {
        return _leftMousePressed;
    }

    bool RightMousePressed() {
        return _rightMousePressed;
    }

    bool MouseWheelDown() {
        return _mouseWheelDown;
    }

    int GetMouseWheelValue() {
        return _mouseWheelValue;
    }

    bool MouseWheelUp() {
        return _mouseWheelUp;
    }

    void PreventRightMouseHold() {
        _preventRightMouseHoldTillNextClick = true;
    }

    int GetMouseX() {
        return (int)_mouseX;
    }

    int GetMouseY() {
        return (int)_mouseY;
    }


    int GetViewportMappedMouseX(int viewportWidth) {
        return Util::MapRange(Input::GetMouseX(), 0, BackEnd::GetCurrentWindowWidth(), 0, viewportWidth);
    }

    int GetViewportMappedMouseY(int viewportHeight) {
        return Util::MapRange(Input::GetMouseY(), 0, BackEnd::GetCurrentWindowHeight(), 0, viewportHeight);
    }


    int GetScrollWheelYOffset() {
        return _scrollWheelYOffset;
    }

    void ResetScrollWheelYOffset() {
        _scrollWheelYOffset = 0;
    }

    /*int GetCursorX() {
        double xpos, ypos;
        glfwGetCursorPos(_window, &xpos, &ypos);
        return int(xpos);
    }

    int GetCursorY() {
        double xpos, ypos;
        glfwGetCursorPos(_window, &xpos, &ypos);
        return int(ypos);
    }*/

    void DisableCursor() {
        glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    void HideCursor() {
        glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    }

    void ShowCursor() {
        glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    int GetCursorScreenX() {
        return _mouseScreenX;
    }

    int GetCursorScreenY() {
        return _mouseScreenY;
    }


    /////////////////////////
    //                     //
    //      Callbacks      //

    void scroll_callback(GLFWwindow* /*window*/, double /*xoffset*/, double yoffset) {
        _scrollWheelYOffset = (int)yoffset;
    }
}