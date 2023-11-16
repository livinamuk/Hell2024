#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "Texture.h"
#include "../Util.hpp"
#include "DDS_Helpers.h"

constexpr uint32_t GL_COMPRESSED_RGB_S3TC_DXT1_EXT = 0x83F0;
constexpr uint32_t GL_COMPRESSED_RGBA_S3TC_DXT1_EXT = 0x83F1;
constexpr uint32_t GL_COMPRESSED_RGBA_S3TC_DXT3_EXT = 0x83F2;
constexpr uint32_t GL_COMPRESSED_RGBA_S3TC_DXT5_EXT = 0x83F3;

uint32_t cmpToOpenGlFormat(CMP_FORMAT format) {
	if (format == CMP_FORMAT_DXT1) {
		return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
	}
	else if (format == CMP_FORMAT_DXT3) {
		return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
	}
	else if (format == CMP_FORMAT_DXT5) {
		return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
	}
	else {
		return 0xFFFFFFFF;
	}
}

void freeCMPTexture(CMP_Texture* t) {
	free(t->pData);
}

Texture::Texture() {
}

Texture::Texture(std::string filepath)
{	
	if (!Util::FileExists(filepath)) {
		std::cout << filepath << " does not exist.\n";
		return;
	}

	_filename = filepath.substr(filepath.rfind("/") + 1);
	_filename = _filename.substr(0, _filename.length() - 4);
	_filetype = filepath.substr(filepath.length() - 3);

	// Check if compressed version exists. If not, create one.
	std::string suffix = _filename.substr(_filename.length() - 3);
	//std::cout << suffix << "\n";
	std::string compressedPath = "res/assets/" + _filename + ".dds";
	if (!Util::FileExists(compressedPath)) {

		stbi_set_flip_vertically_on_load(false);
		_data = stbi_load(filepath.c_str(), &_width, &_height, &_NumOfChannels, 0);

		if (suffix == "NRM") {
			//swizzle
			if (_NumOfChannels == 3) {
				uint8_t* image = _data;
				const uint64_t pitch = static_cast<uint64_t>(_width) * 3UL;
				for (auto r = 0; r < _height; ++r) {
					uint8_t* row = image + r * pitch;
					for (auto c = 0UL; c < static_cast<uint64_t>(_width); ++c) {
						uint8_t* pixel = row + c * 3UL;
						uint8_t  p = pixel[0];
						pixel[0] = pixel[2];
						pixel[2] = p;
					}
				}
			}
			CMP_Texture srcTexture = { 0 };
			srcTexture.dwSize = sizeof(CMP_Texture);
			srcTexture.dwWidth = _width;
			srcTexture.dwHeight = _height;
			srcTexture.dwPitch = _NumOfChannels == 4 ? _width * 4 : _width * 3;
			srcTexture.format = _NumOfChannels == 4 ? CMP_FORMAT_RGBA_8888 : CMP_FORMAT_RGB_888;
			srcTexture.dwDataSize = srcTexture.dwHeight * srcTexture.dwPitch;
			srcTexture.pData = _data;
			CMP_Texture destTexture = { 0 };
			destTexture.dwSize = sizeof(destTexture);
			destTexture.dwWidth = _width;
			destTexture.dwHeight = _height;
			destTexture.dwPitch = _width;
			destTexture.format = CMP_FORMAT_DXT3;
			destTexture.dwDataSize = CMP_CalculateBufferSize(&destTexture);
			destTexture.pData = (CMP_BYTE*)malloc(destTexture.dwDataSize);
			CMP_CompressOptions options = { 0 };
			options.dwSize = sizeof(options);
			options.fquality = 0.88f;
			CMP_ERROR   cmp_status;
			cmp_status = CMP_ConvertTexture(&srcTexture, &destTexture, &options, nullptr);
			if (cmp_status != CMP_OK) {
				free(destTexture.pData);
				std::printf("Compression returned an error %d\n", cmp_status);
				return;
			}
			else {
				SaveDDSFile(compressedPath.c_str(), destTexture);
				free(destTexture.pData);
			}
		}
		else if (suffix == "RMA") {
			//swizzle
			if (_NumOfChannels == 3) {
				uint8_t* image = _data;
				const uint64_t pitch = static_cast<uint64_t>(_width) * 3UL;
				for (auto r = 0; r < _height; ++r) {
					uint8_t* row = image + r * pitch;
					for (auto c = 0UL; c < static_cast<uint64_t>(_width); ++c) {
						uint8_t* pixel = row + c * 3UL;
						uint8_t  p = pixel[0];
						pixel[0] = pixel[2];
						pixel[2] = p;
					}
				}
			}
			CMP_Texture srcTexture = { 0 };
			srcTexture.dwSize = sizeof(CMP_Texture);
			srcTexture.dwWidth = _width;
			srcTexture.dwHeight = _height;
			srcTexture.dwPitch = _NumOfChannels == 4 ? _width * 4 : _width * 3;
			srcTexture.format = _NumOfChannels == 4 ? CMP_FORMAT_RGBA_8888 : CMP_FORMAT_BGR_888;
			srcTexture.dwDataSize = srcTexture.dwHeight * srcTexture.dwPitch;
			srcTexture.pData = _data;
			CMP_Texture destTexture = { 0 };
			destTexture.dwSize = sizeof(destTexture);
			destTexture.dwWidth = _width;
			destTexture.dwHeight = _height;
			destTexture.dwPitch = _width;
			destTexture.format = CMP_FORMAT_DXT3;
			destTexture.dwDataSize = CMP_CalculateBufferSize(&destTexture);
			destTexture.pData = (CMP_BYTE*)malloc(destTexture.dwDataSize);
			CMP_CompressOptions options = { 0 };
			options.dwSize = sizeof(options);
			options.fquality = 0.88f;
			CMP_ERROR   cmp_status;
			cmp_status = CMP_ConvertTexture(&srcTexture, &destTexture, &options, nullptr);
			if (cmp_status != CMP_OK) {
				free(destTexture.pData);
				std::printf("Compression returned an error %d\n", cmp_status);
				return;
			}
			else {
				SaveDDSFile(compressedPath.c_str(), destTexture);
				free(destTexture.pData);
			}
		}
		else if (suffix == "ALB") {
			//swizzle
			if (_NumOfChannels == 3) {
				uint8_t* image = _data;
				const uint64_t pitch = static_cast<uint64_t>(_width) * 3UL;
				for (auto r = 0; r < _height; ++r) {
					uint8_t* row = image + r * pitch;
					for (auto c = 0UL; c < static_cast<uint64_t>(_width); ++c) {
						uint8_t* pixel = row + c * 3UL;
						uint8_t  p = pixel[0];
						pixel[0] = pixel[2];
						pixel[2] = p;
					}
				}
			}
			CMP_Texture srcTexture = { 0 };
			srcTexture.dwSize = sizeof(CMP_Texture);
			srcTexture.dwWidth = _width;
			srcTexture.dwHeight = _height;
			srcTexture.dwPitch = _NumOfChannels == 4 ? _width * 4 : _width * 3;
			srcTexture.format = _NumOfChannels == 4 ? CMP_FORMAT_RGBA_8888 : CMP_FORMAT_RGB_888;
			srcTexture.dwDataSize = srcTexture.dwHeight * srcTexture.dwPitch;
			srcTexture.pData = _data;
			CMP_Texture destTexture = { 0 };
			destTexture.dwSize = sizeof(destTexture);
			destTexture.dwWidth = _width;
			destTexture.dwHeight = _height;
			destTexture.dwPitch = _width;
			destTexture.format = CMP_FORMAT_DXT3;
			destTexture.dwDataSize = CMP_CalculateBufferSize(&destTexture);
			destTexture.pData = (CMP_BYTE*)malloc(destTexture.dwDataSize);
			CMP_CompressOptions options = { 0 };
			options.dwSize = sizeof(options);
			options.fquality = 0.88f;
			CMP_ERROR   cmp_status;
			cmp_status = CMP_ConvertTexture(&srcTexture, &destTexture, &options, nullptr);
			if (cmp_status != CMP_OK) {
				free(destTexture.pData);
				std::printf("Compression returned an error %d\n", cmp_status);
				return;
			}
			else {
				SaveDDSFile(compressedPath.c_str(), destTexture);
				free(destTexture.pData);
			}
		}
	}
	if (suffix == "ALN" || suffix == "NRM" || suffix == "RMA") {
		// Load the compressed version
		CMP_Texture cmpTexture = {};
		LoadDDSFile(compressedPath.c_str(), cmpTexture);
		glGenTextures(1, &_ID);
		glBindTexture(GL_TEXTURE_2D, _ID);
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		uint32_t glFormat = cmpToOpenGlFormat(cmpTexture.format);
		unsigned int blockSize = (glFormat == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) ? 8 : 16;
		if (glFormat != 0xFFFFFFFF) {
			uint32_t width = cmpTexture.dwWidth;
			uint32_t height = cmpTexture.dwHeight;
			uint32_t size1 = ((width + 3) / 4) * ((height + 3) / 4) * blockSize;
			uint32_t size2 = cmpTexture.dwDataSize;
			glCompressedTexImage2D(GL_TEXTURE_2D, 0, glFormat, cmpTexture.dwWidth, cmpTexture.dwHeight, 0, size2, cmpTexture.pData);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 1);
			glGenerateMipmap(GL_TEXTURE_2D);
			freeCMPTexture(&cmpTexture);
			cmpTexture = {};
		}
		return;
	}

	//For everything else just load the raw texture. Compression fucks up UI elements.

	stbi_set_flip_vertically_on_load(false);
	_data = stbi_load(filepath.c_str(), &_width, &_height, &_NumOfChannels, 0);

	glGenTextures(1, &_ID);
	glBindTexture(GL_TEXTURE_2D, _ID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	GLint format = GL_RGB;
	if (_NumOfChannels == 4)
		format = GL_RGBA;
	if (_NumOfChannels == 1)
		format = GL_RED;
	if (_data) {
	//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
		//glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 3);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _width, _height, 0, format, GL_UNSIGNED_BYTE, _data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
		std::cout << "Failed to load texture: " << filepath << "\n";

	stbi_image_free(_data);
}

unsigned int Texture::GetID() {
	return _ID;
}

void Texture::Bind(unsigned int slot) {
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, _ID);
}

int Texture::GetWidth() {
	return _width;
}

int Texture::GetHeight() {
	return _height;
}

std::string& Texture::GetFilename() {
	return _filename;
}
