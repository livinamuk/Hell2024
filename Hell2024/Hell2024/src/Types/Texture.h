#pragma once
#include <Compressonator.h>
//#include <stb_image.h>
#include <string>
#include <memory>
#include "../API/OpenGL/Types/GL_texture.h"
#include "../API/Vulkan/Types/VK_texture.h"

class Texture {

public:
	Texture() = default;
	explicit Texture(const std::string_view filepath);
	void Load(const std::string_view filepath);	
	int GetWidth();
	int GetHeight();
	std::string& GetFilename();
	std::string& GetFiletype();

    OpenGLTexture glTexture;
    VulkanTexture vkTexture;
};
