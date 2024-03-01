#include "Input.h"
#include "GL.h"

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


void Input::Init() {

    double x, y;
    GLFWwindow* window = GL::GetWindowPtr();
    glfwGetCursorPos(window, &x, &y);
    _mouseOffsetX = x;
    _mouseOffsetY = y;
    _mouseX = x;
    _mouseY = y;   
}

void Input::Update() {

    GLFWwindow* window = GL::GetWindowPtr();

    // Wheel
    _mouseWheelUp = false;
    _mouseWheelDown = false;
    _mouseWheelValue = GL::GetScrollWheelYOffset();
    if (_mouseWheelValue < 0)
        _mouseWheelDown = true;
    if (_mouseWheelValue > 0)
        _mouseWheelUp = true;
    GL::ResetScrollWheelYOffset();

    // Keyboard
    for (int i = 32; i < 349; i++) {
        // down
        if (glfwGetKey(window, i) == GLFW_PRESS)
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
    glfwGetCursorPos(window, &x, &y);
    _mouseOffsetX = x - _mouseX;
    _mouseOffsetY = y - _mouseY;
    _mouseX = x;
    _mouseY = y;

    // Left mouse down/pressed
    _leftMouseDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
    if (_leftMouseDown == GLFW_PRESS && !_leftMouseDownLastFrame)
        _leftMousePressed = true;
    else
        _leftMousePressed = false;
    _leftMouseDownLastFrame = _leftMouseDown;

    // Right mouse down/pressed
    _rightMouseDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
    if (_rightMouseDown == GLFW_PRESS && !_rightMouseDownLastFrame)
        _rightMousePressed = true;
    else
        _rightMousePressed = false;
    _rightMouseDownLastFrame = _rightMouseDown;

    if (_rightMousePressed)
        _preventRightMouseHoldTillNextClick = false;
}

bool Input::KeyPressed(unsigned int keycode) {
    return _keyPressed[keycode];
}

bool Input::KeyDown(unsigned int keycode) {
    return _keyDown[keycode];
}

float Input::GetMouseOffsetX() {
    return (float)_mouseOffsetX;
}

float Input::GetMouseOffsetY() {
    return (float)_mouseOffsetY;
}

bool Input::LeftMouseDown() {
    return _leftMouseDown;
}

bool Input::RightMouseDown() {
    return _rightMouseDown && !_preventRightMouseHoldTillNextClick;
}

bool Input::LeftMousePressed() {
    return _leftMousePressed;
}

bool Input::RightMousePressed() {
    return _rightMousePressed;
}

bool Input::MouseWheelDown() {
    return _mouseWheelDown;
}

int Input::GetMouseWheelValue() {
    return _mouseWheelValue;
}

bool Input::MouseWheelUp() {
    return _mouseWheelUp;
}

void Input::PreventRightMouseHold() {
    _preventRightMouseHoldTillNextClick = true;
}

int Input::GetMouseX() {
    return (int)_mouseX;
}

int Input::GetMouseY() {
    return (int)_mouseY;
}