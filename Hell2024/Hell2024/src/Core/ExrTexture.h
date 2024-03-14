#pragma once
#include "../common.h"

class ExrTexture
{
public: // Methods
	ExrTexture();
	ExrTexture(std::string filepath);

	bool GetEXRLayers(const char* filename);
	bool LoadEXRRGBA(float** rgba, int* w, int* h, const char* filename, const char* layername);

	int gWidth = 512;
	int gHeight = 512;
	GLuint gTexId;
	float gIntensityScale = 1.0;
	float gGamma = 1.0;
	int gExrWidth, gExrHeight;
	float* gExrRGBA;
	int gMousePosX, gMousePosY;

	const char* layername = NULL;

public: // fields
/*	unsigned int ID = 0;
	std::string name;
	std::string filetype;
	bool m_readFromDisk = false;
	bool m_loadedToGL = false;
	int width, height;*/

private: // fields
	//unsigned char* data;
	//int nrChannels;
};