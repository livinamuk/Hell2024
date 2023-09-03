#pragma once

class GBuffer
{
public:

	void Configure(int width, int height);
	void Bind();
	void EnableAllDrawBuffers();
	unsigned int GetID();
	unsigned int GetWidth();
	unsigned int GetHeight();

private:
	unsigned int _ID = 0;
	unsigned int _gAlbedoTexture;
	unsigned int _gNormalTexture;
	unsigned int _gDepthTexture;
	int _width;
	int _height;

};
