#pragma once

class GBuffer
{
public:

	void Configure(int width, int height);
	void Bind();
	void Destroy();
	unsigned int GetID();
	unsigned int GetWidth();
	unsigned int GetHeight();
	unsigned int _gBaseColorTexture = { 0 };
	unsigned int _gNormalTexture = { 0 };
	unsigned int _gRMATexture = { 0 };
	unsigned int _gDepthTexture = { 0 };
	unsigned int _gLightingTexture = { 0 };
	unsigned int _gGlassTexture = { 0 };
	unsigned int _gGlassCompositeTemporary = { 0 };
	unsigned int _gWorldSpacePosition = { 0 };
	unsigned int _gEmissive = { 0 };

private:
	unsigned int _ID = { 0 };
	int _width = { 0 };
	int _height = { 0 };;
};
