#include "AssetLoader.h"
#include "tinyexr.h"
#include <stb_image.h>
#include <Compressonator.h>
#include "../BackEnd/Backend.h"
#include "../Util.hpp"
#include "../../vendor/DDS/DDS_Helpers.h"


namespace AssetLoader {

    void LoadGLTextureFromFile(Texture& texture, const std::string filepath);
    void LoadVKTextureFromFile(Texture& texture, const std::string filepath);

    void LoadTextureFromFile(Texture& texture, const std::string filepath) {
        if (BackEnd::GetAPI() == API::OPENGL) {
            LoadGLTextureFromFile(texture, filepath);
        }
        else if (BackEnd::GetAPI() == API::VULKAN) {
            LoadVKTextureFromFile(texture, filepath);
        }
    }

    void LoadGLTextureFromFile(Texture& texture, const std::string filepath) {
        /*

        if (!Util::FileExists(filepath)) {
            std::cout << filepath << " does not exist.\n";
            return;
        }

        texture.SetFilename(Util::GetFilename(filepath));
        texture.SetFiletype(Util::GetFileInfo(filepath).filetype);

        // Is it a VAT texture?
        if (texture.GetFiletype() == "exr") {

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
        return true;*/
    }


    void LoadVKTextureFromFile(Texture& texture, const std::string filepath) {
        // TODO!!!
    }
}