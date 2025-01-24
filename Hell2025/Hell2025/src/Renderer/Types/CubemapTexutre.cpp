#include "CubemapTexture.h"
#include "../../BackEnd/BackEnd.h"

void CubemapTexture::SetName(std::string name) {
    this->name = name;
}

void CubemapTexture::SetFiletype(std::string filetype) {
    this->filetype = filetype;
}

void CubemapTexture::Load() {

    if (BackEnd::GetAPI() == API::OPENGL) {
        glTexture.Load(name, filetype);
    }
    else if (BackEnd::GetAPI() == API::VULKAN) {
        //vkTexture.Load(filepath);
        return;
    }
}

std::string& CubemapTexture::GetName() {
    return name;
}

int CubemapTexture::GetWidth() {
    if (BackEnd::GetAPI() == API::OPENGL) {
        return glTexture.GetWidth();
    }
    else if (BackEnd::GetAPI() == API::VULKAN) {
        //return vkTexture.GetWidth();
    }
}

int CubemapTexture::GetHeight() {
    if (BackEnd::GetAPI() == API::OPENGL) {
        return glTexture.GetHeight();
    }
    else if (BackEnd::GetAPI() == API::VULKAN) {
        //return vkTexture.GetHeight();
    }
}
/*
std::string& CubemapTexture::GetFilename() {
    //return filename;
    if (BackEnd::GetAPI() == API::OPENGL) {
        return glTexture.GetFilename();
    }
    else if (BackEnd::GetAPI() == API::VULKAN) {
        //return vkTexture.GetFilename();
    }
}*/

/*std::string& Texture::GetFiletype() {
    if (BackEnd::GetAPI() == API::OPENGL) {
        return glTexture.GetFiletype();
    }
    else if (BackEnd::GetAPI() == API::VULKAN) {
        return vkTexture.GetFiletype();
    }
}*/

OpenGLCubemapTexture& CubemapTexture::GetGLTexture() {
    return glTexture;
}
/*
VulkanCubemapTexture& CubemapTexture::GetVKTexture() {
    return vkTexture;
}*/