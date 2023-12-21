#pragma once

class PresentFrameBuffer
{
public:

	void Configure(int width, int height);
	void Bind();
	void Destroy();
	unsigned int GetID();
	unsigned int GetWidth();
	unsigned int GetHeight();
	unsigned int _inputTexture = { 0 };
	unsigned int _fxaaTexture = { 0 };
	unsigned int _depthTexture = { 0 };

private:
	unsigned int _ID = { 0 };
	int _width = { 0 };
	int _height = { 0 };;
};
