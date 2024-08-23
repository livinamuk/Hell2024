#pragma once
#include "../../Renderer/RenderData.h"
#include "../../Renderer/RendererCommon.h"

namespace OpenGLRenderer {

    void InitMinimum();
    void RenderLoadingScreen(std::vector<RenderItem2D>& renderItems);
    void RenderFrame(RenderData& renderData);
    void HotloadShaders();
    void BindBindlessTextures();
    void CreatePlayerRenderTargets(int presentWidth, int presentHeight);
    void UpdatePointCloud();
    void PresentFinalImage();
    void RecreateBlurBuffers();
    void QueryAvaliability();

}