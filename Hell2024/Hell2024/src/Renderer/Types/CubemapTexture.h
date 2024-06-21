#pragma once
#include <string>
#include "../../Util.hpp"
#include "../../API/OpenGL/Types/GL_cubemapTexture.h"
//#include "../API/Vulkan/Types/VK_cubeMaptexture.h"

struct CubemapTexture {

public:

    CubemapTexture() = default;
    void Load();
    void SetName(std::string name);
    void SetFiletype(std::string filetype);
    int GetWidth();
    int GetHeight();
    std::string& GetName();
    OpenGLCubemapTexture& GetGLTexture();
    //VulkanCubeMapTexture& GetVKTexture();

private:
    OpenGLCubemapTexture glTexture;
    //VulkanCubeMapTexture vkTexture;
    std::string name;
    std::string filetype;
    int width = 0;
    int height = 0;
    int channelCount = 0;
};
