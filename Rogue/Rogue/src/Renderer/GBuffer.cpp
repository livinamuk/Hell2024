#include "GBuffer.h"
#include "../Common.h"

void GBuffer::Configure(int width, int height) {

	if (_ID == 0) {
		glGenFramebuffers(1, &_ID);
		glGenTextures(1, &_gAlbedoTexture);
		glGenTextures(1, &_gNormalTexture);
		glGenTextures(1, &_gDepthTexture);
		_width = width;
		_height = height;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, _ID);

	glBindTexture(GL_TEXTURE_2D, _gAlbedoTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _gAlbedoTexture, 0);

	glBindTexture(GL_TEXTURE_2D, _gNormalTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, _gNormalTexture, 0);;

	glBindTexture(GL_TEXTURE_2D, _gDepthTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH32F_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_FLOAT_32_UNSIGNED_INT_24_8_REV, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, _gDepthTexture, 0);

	auto fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

void GBuffer::Bind() {
	glBindFramebuffer(GL_FRAMEBUFFER, _ID);
}

void GBuffer::EnableAllDrawBuffers() {
	unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(2, attachments);
}

unsigned int GBuffer::GetID() {
	return _ID;
}

unsigned int GBuffer::GetWidth() {
	return _width;
}

unsigned int GBuffer::GetHeight() {
	return _height;
}
