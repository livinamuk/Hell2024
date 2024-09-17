#pragma once
#include "HellCommon.h"
#include "../../API/OpenGL/Types/GL_texture.h"
#include "../../API/Vulkan/Types/VK_texture.h"

class ExrTexture {

public: 
    ExrTexture() = default;
	void Load(std::string filepath);

	bool GetEXRLayers(const char* filename);
    bool LoadEXRRGBA(float** rgba, int* w, int* h, const char* filename, const char* layername);

    std::string& GetFilename();
    std::string& GetFiletype();
    OpenGLTexture& GetGLTexture();
    VulkanTexture& GetVKTexture();

	int gWidth = 512;
	int gHeight = 512;
	GLuint gTexId;
	float gIntensityScale = 1.0;
	float gGamma = 1.0;
	int gExrWidth, gExrHeight;
	float* gExrRGBA;
	int gMousePosX, gMousePosY;

    std::string _filename;
    std::string _filetype;

private:
    OpenGLTexture glTexture;
    VulkanTexture vkTexture;
};