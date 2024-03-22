#include "Texture.h"
#include "../BackEnd/BackEnd.h"

void Texture::Load(const std::string_view filepath) {
    if (BackEnd::GetAPI() == API::OPENGL) {        
        glTexture.Load(filepath);
    }
    else if (BackEnd::GetAPI() == API::VULKAN) {
        // TO DO
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