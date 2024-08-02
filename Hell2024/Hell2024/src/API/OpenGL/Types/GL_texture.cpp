#include <iostream>
#include "GL_texture.h"
#include "../../../Util.hpp"
#include "../../vendor/DDS/DDS_Helpers.h"
#include <stb_image.h>

GLuint64 OpenGLTexture::GetBindlessID() {
    return bindlessID;
}

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

OpenGLTexture::OpenGLTexture(const std::string filepath) {
    Load(filepath);
}

#include "tinyexr.h"

bool OpenGLTexture::Load(const std::string filepath) {

    if (!Util::FileExists(filepath)) {
        std::cout << filepath << " does not exist.\n";
        return false;
    }

    _filename = Util::GetFilename(filepath);
    _filetype = Util::GetFileInfo(filepath).filetype;

    // Is it a VAT texture?
    if (_filetype == "exr") {

        // Get EXR Layers
        const char* err = nullptr;
        const char** layer_names = nullptr;
        int num_layers = 0;
        bool status = EXRLayers(filepath.c_str(), &layer_names, &num_layers, &err);
        if (err) {
            fprintf(stderr, "EXR error = %s\n", err);
        }
        if (status != 0) {
            fprintf(stderr, "Load EXR err: %s\n", err);
            std::cout << " GetEXRLayers() FAILED \n";
        }
        if (num_layers > 0) {
            fprintf(stdout, "EXR Contains %i Layers\n", num_layers);
            for (int i = 0; i < (int)num_layers; ++i) {
                fprintf(stdout, "Layer %i : %s\n", i + 1, layer_names[i]);
            }
        }
        free(layer_names);

        // Load the RGBA
        int width, height;
        const char* layername = NULL;
        status = LoadEXRWithLayer(&_floatData, &_width, &_height, filepath.c_str(), layername, &err);
        if (err) {
            fprintf(stderr, "EXR error = %s\n", err);
        }
        if (status != 0) {
            fprintf(stderr, "Load EXR err: %s\n", err);
            std::cout << " LoadEXRRGBA() FAILED \n";
        }

        std::cout << "Loaded EXR: " << filepath << "\n";
        return true;
    }







    // Check if compressed version exists. If not, create one.
    std::string suffix = _filename.substr(_filename.length() - 3);
    //std::cout << suffix << "\n";
    std::string compressedPath = "res/assets/" + _filename + ".dds";
    if (!Util::FileExists(compressedPath)) {
        stbi_set_flip_vertically_on_load(false);
        _data = stbi_load(filepath.data(), &_width, &_height, &_NumOfChannels, 0);

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
            _CMP_texture = std::make_unique<CMP_Texture>(0);
            CMP_Texture destTexture = { *_CMP_texture };
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
                _CMP_texture.reset();
                std::printf("Compression returned an error %d\n", cmp_status);
                return false;
            }
            else {
                std::cout << "saving compressed texture: " << compressedPath.c_str() << "\n";
                SaveDDSFile(compressedPath.c_str(), destTexture);
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
            _CMP_texture = std::make_unique<CMP_Texture>(0);
            CMP_Texture destTexture = { *_CMP_texture };
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
                _CMP_texture.reset();
                std::printf("Compression returned an error %d\n", cmp_status);
                return false;
            }
            else {
                SaveDDSFile(compressedPath.c_str(), destTexture);
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
            _CMP_texture = std::make_unique<CMP_Texture>(0);
            CMP_Texture destTexture = { *_CMP_texture };
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
                _CMP_texture.reset();
                std::printf("Compression returned an error %d\n", cmp_status);
                return false;
            }
            else {
                SaveDDSFile(compressedPath.c_str(), destTexture);
            }
        }
    }

    if (_CMP_texture == nullptr) {
        //For everything else just load the raw texture. Compression fucks up UI elements.
        stbi_set_flip_vertically_on_load(false);
        _data = stbi_load(filepath.data(), &_width, &_height, &_NumOfChannels, 0);
    }
    return true;
}


void OpenGLTexture::UploadToGPU(void* data, CMP_Texture* cmpTexture, int width, int height, int channelCount) {

    if (_baked || !_data) {
        return;
    }
    _baked = true;


    if (_filetype == "exr") {
        return;
    }

    if (_CMP_texture != nullptr) {
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        const uint32_t glFormat = cmpToOpenGlFormat(cmpTexture->format);
        if (glFormat != 0xFFFFFFFF) {
            uint32_t size2 = cmpTexture->dwDataSize;
            glCompressedTexImage2D(GL_TEXTURE_2D, 0, glFormat, cmpTexture->dwWidth, cmpTexture->dwHeight, 0, size2, cmpTexture->pData);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 28);
            glGenerateMipmap(GL_TEXTURE_2D);
        }
        freeCMPTexture(cmpTexture);
    }
    else {
        glGenTextures(1, &ID);
        glBindTexture(GL_TEXTURE_2D, ID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        GLint format = GL_RGB;
        if (channelCount == 4)
            format = GL_RGBA;
        if (channelCount == 1)
            format = GL_RED;

        else {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }

        bindlessID = glGetTextureHandleARB(ID);
        glMakeTextureHandleResidentARB(bindlessID);
        //glMakeTextureHandleNonResidentARB(bindlessID); to unbind

    }
    _width = width;
    _height = height;
}

bool OpenGLTexture::Bake() {

    if (_baked) {
        return true;
    }

    _baked = true;

    if (_CMP_texture != nullptr) {
        auto& cmpTexture{ *_CMP_texture };
        glGenTextures(1, &ID);
        glBindTexture(GL_TEXTURE_2D, ID);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        const uint32_t glFormat = cmpToOpenGlFormat(cmpTexture.format);
        //unsigned int blockSize = (glFormat == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) ? 8 : 16;
        if (glFormat != 0xFFFFFFFF) {
            uint32_t size2 = cmpTexture.dwDataSize;
            glCompressedTexImage2D(GL_TEXTURE_2D, 0, glFormat, cmpTexture.dwWidth, cmpTexture.dwHeight, 0, size2, cmpTexture.pData);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 28);
            glGenerateMipmap(GL_TEXTURE_2D);
            cmpTexture = {};
        }
        freeCMPTexture(_CMP_texture.get());
        _CMP_texture.reset();
        return true;
    }

    if (_data == nullptr && _floatData == nullptr) {
        return false;
    }

    glGenTextures(1, &ID);
    glBindTexture(GL_TEXTURE_2D, ID);

    GLint format = GL_RGB;
    if (_NumOfChannels == 4)
        format = GL_RGBA;
    if (_NumOfChannels == 1)
        format = GL_RED;


    if (_filetype == "exr") {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16, _width, _height, 0, GL_RGBA, GL_FLOAT, _floatData);
        free(_floatData);
    }
    else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _width, _height, 0, format, GL_UNSIGNED_BYTE, _data);
        //stbi_image_free(_data);
        std::cout << "WARNING: u didn't call stbi_image_free(_data) on: '" + GetFilename() << "'\n";
        // find out why the line above crashes sometimes. most likely an async mutex thing.
        // ..
        // well actually no it cant be
        // coz its on the EXR textures and they aint loaded async
    }

    // Hack to make Resident Evil font look okay when scaled
    std::string filename = this->GetFilename();
    if (filename.substr(0, 4) == "char") {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    }

    bindlessID = glGetTextureHandleARB(ID);
    glMakeTextureHandleResidentARB(bindlessID);
    //glMakeTextureHandleNonResidentARB(bindlessID); to unbind

    _data = nullptr;
    return true;
}

unsigned int OpenGLTexture::GetID() {
    return ID;
}

void OpenGLTexture::Bind(unsigned int slot) {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, ID);
}

int OpenGLTexture::GetWidth() {
    return _width;
}

int OpenGLTexture::GetHeight() {
    return _height;
}

std::string& OpenGLTexture::GetFilename() {
    return _filename;
}

std::string& OpenGLTexture::GetFiletype() {
    return _filetype;
}

bool OpenGLTexture::IsBaked() {
    return _baked;
}
