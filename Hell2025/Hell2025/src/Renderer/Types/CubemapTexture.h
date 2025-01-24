#pragma once
#include <string>
#include "../../Util.hpp"
#include "../../API/OpenGL/Types/GL_cubemapTexture.h"
//#include "../API/Vulkan/Types/VK_cubeMaptexture.h"

struct CubemapTexture {

public:

    CubemapTexture() = default;
    CubemapTexture(std::string fullPath) {
        m_fullPath = fullPath;
    }
    void Load();
    void SetName(std::string name);
    void SetFiletype(std::string filetype);
    int GetWidth();
    int GetHeight();
    std::string& GetName();
    OpenGLCubemapTexture& GetGLTexture();
    //VulkanCubeMapTexture& GetVKTexture();

    bool m_awaitingLoadingFromDisk = true;
    bool m_loadedFromDisk = false;
    std::string m_fullPath = "";
    bool m_baked = false;

private:
    OpenGLCubemapTexture glTexture;
    //VulkanCubeMapTexture vkTexture;
    std::string name;
    std::string filetype;
    int width = 0;
    int height = 0;

};
