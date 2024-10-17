#pragma once
#include <Compressonator.h>
#include <string>
#include <memory>
#include "../../API/OpenGL/Types/GL_texture.h"
#include "../../API/Vulkan/Types/VK_texture.h"
#include "../../Types/Enums.h"

class Texture {

public:

    Texture() = default;
    Texture(std::string fullpath, bool compressed);
	void Load();
    void Bake();
    void BakeCMPData(CMP_Texture* cmpTexture);
	int GetWidth();
	int GetHeight();
	std::string& GetFilename();
	std::string& GetFiletype();
    OpenGLTexture& GetGLTexture();
    VulkanTexture& GetVKTexture();

    void SetLoadingState(LoadingState loadingState);
    const LoadingState GetLoadingState();
    const BakingState GetBakingState();

    std::string m_fullPath = "";
    bool m_compressed = false;

private:
    OpenGLTexture glTexture;
    VulkanTexture vkTexture;
    std::string m_fileName;
    std::string m_fileType;
    int width = 0;
    int height = 0;
    int channelCount = 0;
    LoadingState m_loadingState = LoadingState::AWAITING_LOADING_FROM_DISK;
    BakingState m_bakingState = BakingState::AWAITING_BAKE;
};
