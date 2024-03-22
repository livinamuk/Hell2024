#pragma once
#include "../../Renderer/RendererCommon.h"

namespace OpenGLRenderer {

    void InitMinimum();
    void RenderLoadingScreen();
    void RenderWorld(std::vector<RenderItem3D>& renderItems);
    void RenderUI(std::vector<RenderItem2D>& renderItems);
    void HotloadShaders();
}