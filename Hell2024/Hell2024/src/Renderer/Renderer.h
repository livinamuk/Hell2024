#pragma once
#include "RendererCommon.h"

namespace Renderer {

    void RenderLoadingScreen();
    void RenderFrame();
    void HotloadShaders();
    void NextRenderMode();
    void PreviousRenderMode();
    void NextDebugLineRenderMode();
    void UpdatePointCloud();
    void RecreateBlurBuffers();
    RenderMode GetRenderMode();
    void ToggleProbes();
    bool ProbesVisible();
}