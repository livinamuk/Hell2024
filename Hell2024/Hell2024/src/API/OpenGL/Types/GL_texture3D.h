#pragma once
#include <string>

class OpenGLTexture3D
{
public: // Methods
    OpenGLTexture3D();
    void Create(int width, int height, int depth);
	void Bind(unsigned int slot);
    void CleanUp();
	unsigned int GetID();
	unsigned int GetWidth();
	unsigned int GetHeight();
	unsigned int GetDepth();

private:
	unsigned int m_ID = 0;
    int m_width = 0;
    int m_height = 0;
    int m_depth = 0;
};
