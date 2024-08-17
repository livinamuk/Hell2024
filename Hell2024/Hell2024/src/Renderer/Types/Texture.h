#pragma once
#include <Compressonator.h>
#include <string>
#include <memory>
#include "../../API/OpenGL/Types/GL_texture.h"
#include "../../API/Vulkan/Types/VK_texture.h"

class Texture {

public:

    Texture() = default;
    Texture(std::string fullpath) {
        m_fullPath = fullpath;
    }
    /*
    Texture(std::string filename, int width, int height, int channelCount) {
        this->width = width;
        this->height = height;
        this->channelCount = channelCount;
        this->filename = filename;
    }*/

    //void UploadToGPU(void* data, CMP_Texture* cmpTexture, int width, int height, int channelCount);
	void Load(const std::string filepath);
	int GetWidth();
	int GetHeight();
	std::string& GetFilename();
	std::string& GetFiletype();
    OpenGLTexture& GetGLTexture();
    VulkanTexture& GetVKTexture();

    void SetFilename(std::string name) {
        filename = name;
    }
    void SetFiletype(std::string type) {
        filetype = type;
    }
    bool m_loadingBegan = false;
    bool m_loadingComplete = false;
    bool m_baked = false;

    std::string m_fullPath = "";

private:
    OpenGLTexture glTexture;
    VulkanTexture vkTexture;
    std::string filename;
    std::string filetype;
    int width = 0;
    int height = 0;
    int channelCount = 0;
};
