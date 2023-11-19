#include "GL.h"
#include <iostream>

namespace GL {
    inline GLFWwindow* _window;
    inline GLFWmonitor* _monitor;
    inline const GLFWvidmode* _mode;
    inline int _currentWidth = 0;
    inline int _currentHeight = 0;
    inline int _windowedtWidth = 0;
    inline int _windowedHeight = 0;
    inline int _fullscreenWidth = 0;
    inline int _fullscreenHeight = 0;
    inline int _mouseScreenX = 0;
    inline int _mouseScreenY = 0;
    inline int _windowHasFocus = true;
    inline bool _forceCloseWindow = false;
    inline int _scrollWheelYOffset = 0;
    inline WindowMode _windowMode = WINDOWED;// FULLSCREEN;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
void window_focus_callback(GLFWwindow* window, int focused);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

//void APIENTRY glDebugOutput(GLenum type, unsigned int id, GLenum severity, GLsizei length, const char* message, const void* userParam);

GLenum glCheckError_(const char* file, int line)
{
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR)
    {
        std::string error;
        switch (errorCode)
        {
        case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
        case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
        case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
        case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
        case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
        case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
        case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
        }
        std::cout << error << " | " << file << " (" << line << ")" << std::endl;
    }
    return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void APIENTRY glDebugOutput(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length, const char* message, const void* userParam)
{
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return; // ignore these non-significant error codes

    std::cout << "---------------" << std::endl;
    std::cout << "Debug message (" << id << "): " << message << std::endl;

    switch (source)
    {
    case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
    case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
    case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
    case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
    } std::cout << std::endl;

    switch (type)
    {
    case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break;
    case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
    case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
    case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
    case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
    case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
    case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
    } std::cout << std::endl;

    switch (severity)
    {
    case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
    case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
    case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
    case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
    } std::cout << std::endl;
    std::cout << std::endl;
    std::cout << std::endl;
}

void GL::ToggleFullscreen() {
    if (_windowMode == WINDOWED)
        SetWindowMode(FULLSCREEN);
    else
        SetWindowMode(WINDOWED);
}

int GL::GetScrollWheelYOffset() {
    return _scrollWheelYOffset;
}

void GL::ResetScrollWheelYOffset() {
    _scrollWheelYOffset = 0;
}

void GL::CreateWindow(WindowMode windowMode) {
    if (windowMode == WINDOWED) {
        _currentWidth = _windowedtWidth;
        _currentHeight = _windowedHeight;
        _window = glfwCreateWindow(_windowedtWidth, _windowedHeight, "Rogue", NULL, NULL);
    }
    else if (windowMode == FULLSCREEN) {
        _currentWidth = _fullscreenWidth;
        _currentHeight = _fullscreenHeight;
        _window = glfwCreateWindow(_fullscreenWidth, _fullscreenHeight, "Noose Girl", _monitor, NULL);
    }
    _windowMode = windowMode;
}

void GL::SetWindowMode(WindowMode windowMode) {
    if (windowMode == WINDOWED) {
        _currentWidth = _windowedtWidth;
        _currentHeight = _windowedHeight;
        glfwSetWindowMonitor(_window, nullptr, 0, 0, _windowedtWidth, _windowedHeight, 0);
    } 
    else if (windowMode == FULLSCREEN) {
        _currentWidth = _fullscreenWidth;
        _currentHeight = _fullscreenHeight;
        glfwSetWindowMonitor(_window, _monitor, 0, 0, _fullscreenWidth, _fullscreenHeight, _mode->refreshRate);
    }
    _windowMode = windowMode;
}

void QuerySizes() {
    GLint max_layers;
    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &max_layers);
    std::cout << "Max texture array size is: " << max_layers << "\n";
    int max_compute_work_group_count[3];
    int max_compute_work_group_size[3];
    int max_compute_work_group_invocations;
    for (int idx = 0; idx < 3; idx++) {
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, idx, &max_compute_work_group_count[idx]);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, idx, &max_compute_work_group_size[idx]);
    }
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &max_compute_work_group_invocations);
    std::cout << "Max number of work groups in X dimension " << max_compute_work_group_count[0] << std::endl;
    std::cout << "Max number of work groups in Y dimension " << max_compute_work_group_count[1] << std::endl;
    std::cout << "Max number of work groups in Z dimension " << max_compute_work_group_count[2] << std::endl;
    std::cout << "Max size of a work group in X dimension " << max_compute_work_group_size[0] << std::endl;
    std::cout << "Max size of a work group in Y dimension " << max_compute_work_group_size[1] << std::endl;
    std::cout << "Max size of a work group in Z dimension " << max_compute_work_group_size[2] << std::endl;
    std::cout << "Number of invocations in a single local work group that may be dispatched to a compute shader " << max_compute_work_group_invocations << std::endl;

}

void GL::Init(int width, int height) {

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true); 
   
    // Resolution and window size
    _monitor = glfwGetPrimaryMonitor();
    _mode = glfwGetVideoMode(_monitor);
    glfwWindowHint(GLFW_RED_BITS, _mode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, _mode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, _mode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, _mode->refreshRate);
    _fullscreenWidth = _mode->width;
    _fullscreenHeight = _mode->height;
    _windowedtWidth = width;
    _windowedHeight = height;

    if (_windowMode == FULLSCREEN) {
        _currentWidth = _fullscreenWidth;
        _currentHeight = _fullscreenHeight;
        _window = glfwCreateWindow(_fullscreenWidth, _fullscreenHeight, "Noose Girl", _monitor, NULL);
    } 
    else {
        _currentWidth = _windowedtWidth;
        _currentHeight = _windowedHeight;
        _window = glfwCreateWindow(_windowedtWidth, _windowedHeight, "Rogue", NULL, NULL);
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
    glfwSetScrollCallback(_window, scroll_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return;
    }

    int flags;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
        //std::cout << "Debug GL context enabled\n";
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // makes sure errors are displayed synchronously
        glDebugMessageCallback(glDebugOutput, nullptr);
    } else {
        std::cout << "Debug GL context not avaliable\n";
    }

    //glDebugMessageCallback(glDebugOutput, nullptr);

    
    glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Cap to 60fps
    //glfwSwapInterval(1);
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
    return _currentWidth;
}

int GL::GetWindowHeight() {
    return _currentHeight;
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

void GL::ShowCursor() {
    glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
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

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    GL::_scrollWheelYOffset = yoffset;
}