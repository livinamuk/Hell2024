#include "Texture.h"
#include "../../BackEnd/BackEnd.h"
#include "../../Util.hpp"

Texture::Texture(std::string fullpath, bool compressed) {
    m_compressed = compressed;
    m_fullPath = fullpath;
    m_fileName = Util::GetFilename(m_fullPath);
    m_fileType = Util::GetFileInfo(m_fullPath).filetype;
}

void Texture::Load() {
    if (BackEnd::GetAPI() == API::OPENGL) {
        glTexture.Load(m_fullPath, m_compressed);
    }
    else if (BackEnd::GetAPI() == API::VULKAN) {
        vkTexture.Load(m_fullPath);
    }
    m_loadingState = LoadingState::LOADING_COMPLETE;
    return;
}

void Texture::Bake() {
    if (m_bakingState == BakingState::AWAITING_BAKE) {
        if (BackEnd::GetAPI() == API::OPENGL) {
            glTexture.Bake();
        }
        if (BackEnd::GetAPI() == API::VULKAN) {
           vkTexture.Bake();
        }
    }
    m_bakingState = BakingState::BAKE_COMPLETE;
}

void Texture::BakeCMPData(CMP_Texture* cmpTexture) {
    if (m_bakingState == BakingState::AWAITING_BAKE) {
        if (BackEnd::GetAPI() == API::OPENGL) {
            glTexture.BakeCMPData(cmpTexture);
        }
        if (BackEnd::GetAPI() == API::VULKAN) {
            //vkTexture.BakeCMPData(cmpTexture);
        }
    }
    m_bakingState = BakingState::BAKE_COMPLETE;
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
    return m_fileName;
    /*if (BackEnd::GetAPI() == API::OPENGL) {
        return glTexture.GetFilename();
    }
    else if (BackEnd::GetAPI() == API::VULKAN) {
        return vkTexture.GetFilename();
    }*/
}

std::string& Texture::GetFiletype() {
    return m_fileType;
    /*
    if (BackEnd::GetAPI() == API::OPENGL) {
        return glTexture.GetFiletype();
    }
    else if (BackEnd::GetAPI() == API::VULKAN) {
        return vkTexture.GetFiletype();
    }*/
}

OpenGLTexture& Texture::GetGLTexture() {
    return glTexture;
}

VulkanTexture& Texture::GetVKTexture() {
    return vkTexture;
}

void Texture::SetLoadingState(LoadingState loadingState) {
    m_loadingState = loadingState;
}

const LoadingState Texture::GetLoadingState() {
    return m_loadingState;
}

const BakingState Texture::GetBakingState() {
    return m_bakingState;
}