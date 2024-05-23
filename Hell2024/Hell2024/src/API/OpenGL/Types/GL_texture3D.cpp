#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "GL_texture3D.h"
#include "../../../Util.hpp"

OpenGLTexture3D::OpenGLTexture3D() {}

void OpenGLTexture3D::Create(int width, int height, int depth){
	_width = width;
	_height = height;
	_depth = depth;
	glGenTextures(1, &_ID);
	glBindTexture(GL_TEXTURE_3D, _ID);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F, _width, _height, _depth, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
}

unsigned int OpenGLTexture3D::GetID() {
	return _ID;
}

unsigned int OpenGLTexture3D::GetWidth() {
	return _width;
}

unsigned int OpenGLTexture3D::GetHeight() {
	return _height;
}

unsigned int OpenGLTexture3D::GetDepth() {
	return _depth;
}

void OpenGLTexture3D::Bind(unsigned int slot) {
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_3D, _ID);
}