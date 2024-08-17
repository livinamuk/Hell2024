#include "Texture.h"
#include "../../BackEnd/BackEnd.h"

void Texture::Load(const std::string filepath) {
    if (BackEnd::GetAPI() == API::OPENGL) {
        glTexture.Load(filepath);
        m_loadingComplete = true;
        return;
    }
    else if (BackEnd::GetAPI() == API::VULKAN) {
        vkTexture.Load(filepath);
        m_loadingComplete = true;
        return;
    }
}

int Texture::GetWidth() {
    if (BackEnd::GetAPI() == API::OPENGL) {
        return glTexture.GetWidth();
    }
    else if (BackEnd::GetAPI() == API::VULKAN) {
        return vkTexture.GetWidth();
    }
}

int Texture::GetHeight() {
    if (BackEnd::GetAPI() == API::OPENGL) {
        return glTexture.GetHeight();
    }
    else if (BackEnd::GetAPI() == API::VULKAN) {
        return vkTexture.GetHeight();
    }
}

std::string& Texture::GetFilename() {
    //return filename;
    if (BackEnd::GetAPI() == API::OPENGL) {
        return glTexture.GetFilename();
    }
    else if (BackEnd::GetAPI() == API::VULKAN) {
        return vkTexture.GetFilename();
    }
}

std::string& Texture::GetFiletype() {
    if (BackEnd::GetAPI() == API::OPENGL) {
        return glTexture.GetFiletype();
    }
    else if (BackEnd::GetAPI() == API::VULKAN) {
        return vkTexture.GetFiletype();
    }
}

OpenGLTexture& Texture::GetGLTexture() {
    return glTexture;
}

VulkanTexture& Texture::GetVKTexture() {
    return vkTexture;
}