#pragma once
#include <string>

struct OpenGLCubemapTexture {

public:
    OpenGLCubemapTexture() = default;
    void LoadAndBake(std::string name, std::string filetype);
    void Bind(unsigned int slot);
    unsigned int GetID();
    unsigned int GetWidth();
    unsigned int GetHeight();

private:
    unsigned int ID = 0;
    int width = 0;
    int height = 0;

};
