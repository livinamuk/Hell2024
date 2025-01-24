#pragma once
#include "HellCommon.h"
#include <string>

struct OpenGLCubemapTexture {

public:
    OpenGLCubemapTexture() = default;
    void Load(std::string name, std::string filetype);
    void Bake();
    void Bind(unsigned int slot);
    unsigned int GetID();
    unsigned int GetWidth();
    unsigned int GetHeight();

    TextureData m_textureData[6];

private:
    unsigned int ID = 0;
    int width = 0;
    int height = 0;

};
