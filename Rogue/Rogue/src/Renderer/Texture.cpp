#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "Texture.h"
#include "../Util.hpp"

Texture::Texture() {
}

Texture::Texture(std::string filepath)
{
	if (!Util::FileExists(filepath)) {
		std::cout << filepath << " does not exist.\n";
		return;
	}

	_filename = filepath.substr(filepath.rfind("/") + 1);
	_filename = _filename.substr(0, _filename.length() - 4);
	_filetype = filepath.substr(filepath.length() - 3);

	stbi_set_flip_vertically_on_load(false);
	_data = stbi_load(filepath.c_str(), &_width, &_height, &_NumOfChannels, 0);

	glGenTextures(1, &_ID);
	glBindTexture(GL_TEXTURE_2D, _ID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	GLint format = GL_RGB;
	if (_NumOfChannels == 4)
		format = GL_RGBA;
	if (_NumOfChannels == 1)
		format = GL_RED;
	if (_data) {
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
		//glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 3);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _width, _height, 0, format, GL_UNSIGNED_BYTE, _data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
		std::cout << "Failed to load texture: " << filepath << "\n";

	stbi_image_free(_data);
}

unsigned int Texture::GetID() {
	return _ID;
}

void Texture::Bind(unsigned int slot) {
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, _ID);
}

int Texture::GetWidth() {
	return _width;
}

int Texture::GetHeight() {
	return _height;
}

std::string& Texture::GetFilename() {
	return _filename;
}
