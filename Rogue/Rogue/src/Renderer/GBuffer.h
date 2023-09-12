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
	unsigned int _ID = { 0 };
	unsigned int _gAlbedoTexture = { 0 };
	unsigned int _gNormalTexture = { 0 };
	unsigned int _gDepthTexture = { 0 };
	int _width = { 0 };
	int _height = { 0 };;

};
