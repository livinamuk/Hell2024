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
    DebugLineRenderMode GetDebugLineRenderMode();
    void ToggleProbes();
    bool ProbesVisible();
}