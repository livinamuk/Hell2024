#pragma once
#include "HellCommon.h"
#include <vector>

struct CubeMap2 {
    void Init(unsigned int size);
    void Clear();
    void CheckStatus();

    unsigned int m_ID = 0;
    unsigned int m_depthTex = 0;
    unsigned int m_colorTex = 0;
    unsigned int m_textureView = 0;
    unsigned int m_size = 0;
};
