#pragma once
#include "HellCommon.h"
#include <vector>

struct ShadowMapArray {
    void Init(unsigned int numberOfCubemaps);
    void CleanUp();
    void Clear();
    GLuint GetDepthTexture() const { return m_depthTexture; }
    GLuint GetTextureView() const { return m_textureView; }

    unsigned int m_ID = 0;
    unsigned int m_depthTexture = 0;
    unsigned int m_numberOfCubemaps = 0;
    unsigned int m_textureView = 0;
};