#include "Texture3D.h"
#include "../../BackEnd/BackEnd.h"

int Texture3D::GetWidth() {
    if (BackEnd::GetAPI() == API::OPENGL) {
        return glTexture3D.GetWidth();
    }
    else if (BackEnd::GetAPI() == API::VULKAN) {
        return vkTexture3D.GetWidth();
    }
}

int Texture3D::GetHeight() {
    if (BackEnd::GetAPI() == API::OPENGL) {
        return glTexture3D.GetHeight();
    }
    else if (BackEnd::GetAPI() == API::VULKAN) {
        return vkTexture3D.GetHeight();
    }
}

int Texture3D::GetDepth() {
    if (BackEnd::GetAPI() == API::OPENGL) {
        return glTexture3D.GetDepth();
    }
    else if (BackEnd::GetAPI() == API::VULKAN) {
        return vkTexture3D.GetDepth();
    }
}

void Texture3D::CleanUp() {

    if (BackEnd::GetAPI() == API::OPENGL) {
        glTexture3D.CleanUp();
    }
    else if (BackEnd::GetAPI() == API::VULKAN) {
        // TO DO
    }
}

OpenGLTexture3D& Texture3D::GetGLTexture3D() {
    return glTexture3D;
}

VulkanTexture3D& Texture3D::GetVKTexture3D() {
    return vkTexture3D;
}