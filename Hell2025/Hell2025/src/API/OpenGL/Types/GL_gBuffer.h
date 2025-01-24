#pragma once
#include <unordered_map>
#include <glad/glad.h>

class GBuffer
{
public:
	void Configure(int width, int height);
	void Bind();
	void Destroy();
	unsigned int GetID();
	unsigned int GetWidth();
	unsigned int GetHeight();
	unsigned int baseColorTexture = { 0 };
	unsigned int normalTexture = { 0 };
	unsigned int RMATexture = { 0 };
	unsigned int depthTexture = { 0 };
	unsigned int lightingTexture = { 0 };
	unsigned int glassTexture = { 0 };
	unsigned int glassCompositeTemporaryTexture = { 0 };
	unsigned int emissiveTexture = { 0 };
    std::unordered_map<GLuint, GLenum> attachments;

private:
	unsigned int ID = { 0 };
	int width = { 0 };
	int height = { 0 };;
};
