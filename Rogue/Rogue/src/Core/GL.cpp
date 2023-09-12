#include "GL.h"
#include <iostream>

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
void window_focus_callback(GLFWwindow* window, int focused);

namespace GL {
    inline GLFWwindow* _window;
    inline int _width = 0;
    inline int _height = 0;
    inline int _mouseScreenX = 0;
    inline int _mouseScreenY = 0;
    inline int _windowHasFocus = true;
    inline bool _forceCloseWindow = false;
}

void GL::Init(int width, int height) {

    _width = width;
    _height = height;

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    bool fullscreen = true;
    if (fullscreen) {
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        glfwWindowHint(GLFW_RED_BITS, mode->redBits);
        glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
        glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
        glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
        _window = glfwCreateWindow(mode->width, mode->height, "Noose Girl", monitor, NULL);
        _width = mode->width;
        _height = mode->height;
    } 
    else {
        _window = glfwCreateWindow(width, height, "Rogue", NULL, NULL);
    }

    if (_window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return;
    }
    glfwMakeContextCurrent(_window);
    glfwSetFramebufferSizeCallback(_window, framebuffer_size_callback);
    glfwSetKeyCallback(_window, key_callback); 
    glfwSetCursorPosCallback(_window, cursor_position_callback);
    glfwSetWindowFocusCallback(_window, window_focus_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return;
    }
    
    glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void GL::ProcessInput() {
    processInput(_window);
}

void GL::SwapBuffersPollEvents() {
    glfwSwapBuffers(_window);
    glfwPollEvents();
}

void GL::Cleanup() {
    glfwTerminate();
}

bool GL::WindowIsOpen() {
    return !glfwWindowShouldClose(_window);
}

int GL::GetWindowWidth() {
    return _width;
}

int GL::GetWindowHeight() {
    return _height;
}

int GL::GetCursorX() {
    double xpos, ypos;
    glfwGetCursorPos(_window, &xpos, &ypos);
    return int(xpos);
}

int GL::GetCursorY() {
    double xpos, ypos;
    glfwGetCursorPos(_window, &xpos, &ypos);
    return int(ypos);
}

void GL::DisableCursor() {
    glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void GL::HideCursor() {
    glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
}

GLFWwindow* GL::GetWindowPtr() {
    return _window;
}

int GL::GetCursorScreenX() {
    return _mouseScreenX;
}

int GL::GetCursorScreenY() {
    return _mouseScreenY;
}

bool GL::WindowHasFocus() {
    return _windowHasFocus;
}

bool GL::WindowHasNotBeenForceClosed() {
    return !_forceCloseWindow;
}

void GL::ForceCloseWindow() {
    _forceCloseWindow = true;
}

// Callbacks

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    /*if (key == GLFW_KEY_H) {
        std::cout << "test\n";
    }*/
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    //std::cout << xpos << "\n";
    /*
    // these three lines were default at event.c code
    Slot* slot = static_cast<Slot*>(glfwGetWindowUserPointer(window));
    printf("%08x to %i at %0.3f: Cursor position: %f %f\n",
        counter++, slot->number, glfwGetTime(), xpos, ypos);

    // these code copied from "cursorPosCallback" default optix sample function, commented above
    Params* params = static_cast<Params*>(glfwGetWindowUserPointer(window));

    if (mouse_button == GLFW_MOUSE_BUTTON_LEFT)
    {
        trackball.setViewMode(sutil::Trackball::LookAtFixed);
        trackball.updateTracking(static_cast<int>(xpos), static_cast<int>(ypos), params->width, params->height);
        camera_changed = true;
    }
    else if (mouse_button == GLFW_MOUSE_BUTTON_RIGHT)
    {
        trackball.setViewMode(sutil::Trackball::EyeFixed);
        trackball.updateTracking(static_cast<int>(xpos), static_cast<int>(ypos), params->width, params->height);
        camera_changed = true;
    }*/
}

void window_focus_callback(GLFWwindow* window, int focused) {
    if (focused){
        GL::_windowHasFocus = true;
    }
    else{
        GL::_windowHasFocus = false;
    }
}