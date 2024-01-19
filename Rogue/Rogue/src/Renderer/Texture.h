#pragma once
#include <Compressonator.h>
#include <stb_image.h>
#include <string>
#include <memory>

class Texture
{
public: // Methods
	Texture() = default;
	explicit Texture(const std::string_view filepath);
	bool Load(const std::string_view filepath, const bool bake = true);
	bool Bake();
	void Bind(unsigned int slot);
	unsigned int GetID();
	int GetWidth();
	int GetHeight();
	std::string& GetFilename();

private:
	unsigned int _ID = 0;
	std::string _filename;
	std::string _filetype;
	std::unique_ptr<CMP_Texture> _CMP_texture;
	unsigned char* _data = nullptr;
	int _NumOfChannels = 0;
	int _width = 0;
	int _height = 0;
};
