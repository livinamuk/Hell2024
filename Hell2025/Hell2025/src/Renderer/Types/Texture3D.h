#pragma once
#include "../../API/OpenGL/Types/GL_texture3D.h"
#include "../../API/Vulkan/Types/VK_texture3D.h"

struct Texture3D {

public:
    Texture3D() = default;
    Texture3D(int width, int height, int depth) {
        this->width = width;
        this->height = height;
        this->depth = depth;
    }
    void Load(const std::string filepath);
    void CleanUp();
    int GetWidth();
    int GetHeight();
    int GetDepth();
    OpenGLTexture3D& GetGLTexture3D();
    VulkanTexture3D& GetVKTexture3D();

private:
    OpenGLTexture3D glTexture3D;
    VulkanTexture3D vkTexture3D;
    int width = 0;
    int height = 0;
    int depth = 0;
};
