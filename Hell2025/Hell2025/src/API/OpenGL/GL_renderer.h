#pragma once
#include "HellCommon.h"
#include "../../Renderer/RenderData.h"
#include "Types/GL_frameBuffer.hpp"
#include "Types/GL_shader.h"

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

    // Move back to GL_renderer.cpp once hair is done
    void HairPass();
    void RenderHairLayer(std::vector<HairRenderItem>& renderItems, int peelCount);

    GLFrameBuffer& GetHairFrameBuffer();
    GLFrameBuffer& GetGBuffer();
    Shader& GetDepthPeelDepthShader();
    Shader& GetDepthPeelColorShader();
    Shader& GetSolidColorShader();
    ComputeShader& GetHairLayerCompositeShader();
    ComputeShader& GetHairFinalCompositeShader();

    void CopyDepthBuffer(GLFrameBuffer& srcFrameBuffer, GLFrameBuffer& dstFrameBuffer);
    void CopyColorBuffer(GLFrameBuffer& srcFrameBuffer, GLFrameBuffer& dstFrameBuffer, const char* srcAttachmentName, const char* dstAttachmentName);
}