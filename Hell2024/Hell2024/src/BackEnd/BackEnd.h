#pragma once
#include "HellCommon.h"

namespace BackEnd {

    // Core
    void Init(API api);
    void BeginFrame();
    void UpdateSubSystems();
    void EndFrame();
    void CleanUp();

    // API
    void SetAPI(API api);
    const API GetAPI();

    // Window
    GLFWwindow* GetWindowPointer();
    const WindowedMode& GetWindowMode();
    void SetWindowPointer(GLFWwindow* window);
    void CreateGLFWWindow(const WindowedMode& windowedMode);
    void SetWindowedMode(const WindowedMode& windowedMode);
    void ToggleFullscreen();
    void ForceCloseWindow();
    bool WindowIsOpen();
    bool WindowHasFocus();
    bool WindowHasNotBeenForceClosed();
    bool WindowIsMinimized();
    int GetWindowedWidth();
    int GetWindowedHeight();
    int GetCurrentWindowWidth();
    int GetCurrentWindowHeight();
    int GetFullScreenWidth();
    int GetFullScreenHeight();

    // Render Targets
    void SetPresentTargetSize(int width, int height);
    int GetPresentTargetWidth();
    int GetPresentTargetHeight();
}