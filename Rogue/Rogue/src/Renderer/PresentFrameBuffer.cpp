#include "PresentFrameBuffer.h"
#include "../Common.h"

void PresentFrameBuffer::Configure(int width, int height) {

	if (_ID == 0) {
		glGenFramebuffers(1, &_ID);
		glGenTextures(1, &_inputTexture);
		glGenTextures(1, &_fxaaTexture);
		glGenTextures(1, &_depthTexture);
		_width = width; 
		_height = height;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, _ID);

	glBindTexture(GL_TEXTURE_2D, _inputTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _inputTexture, 0);

	glBindTexture(GL_TEXTURE_2D, _fxaaTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, _fxaaTexture, 0);

	glBindTexture(GL_TEXTURE_2D, _depthTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH32F_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, _depthTexture, 0);

	auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PresentFrameBuffer::Bind() {
	glBindFramebuffer(GL_FRAMEBUFFER, _ID);
}

void PresentFrameBuffer::Destroy() {
	glDeleteTextures(1, &_inputTexture);
	glDeleteTextures(1, &_fxaaTexture);
	glDeleteTextures(1, &_depthTexture);
	glDeleteFramebuffers(1, &_ID);
}

unsigned int PresentFrameBuffer::GetID() {
	return _ID;
}

unsigned int PresentFrameBuffer::GetWidth() {
	return _width;
}

unsigned int PresentFrameBuffer::GetHeight() {
	return _height;
}
