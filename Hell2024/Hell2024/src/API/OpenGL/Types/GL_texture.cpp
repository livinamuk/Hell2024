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



TextureData LoadEXRData(std::string filepath);
TextureData LoadTextureData(std::string filepath);
TextureData LoadDDSTextureData(std::string filepath);


CompressedTextureData LoadCompressedDDSFromDisk(const char* filepath);

CompressedTextureData LoadCompressedDDSFromDisk(const char* filepath) {
    CompressedTextureData textureData;
    gli::texture texture = gli::load(filepath);
    if (texture.empty()) {
        std::cerr << "Failed to load compressed DDS texture: " << filepath << std::endl;
        return textureData;
    }
    gli::gl GL(gli::gl::PROFILE_GL33);
    textureData.data = (void*)texture.data(0, 0, 0);
    textureData.format = GL.translate(texture.format(), texture.swizzles()).Internal;
    textureData.width = static_cast<GLsizei>(texture.extent().x);
    textureData.height = static_cast<GLsizei>(texture.extent().y);
    textureData.size = static_cast<GLsizei>(texture.size(0));
    textureData.target = texture.target();

    return textureData;
}


TextureData LoadEXRData(std::string filepath) {
    TextureData textureData;
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
    const char* layername = NULL;
    float* floatPtr = nullptr;
    status = LoadEXRWithLayer(&floatPtr, &textureData.m_width, &textureData.m_height, filepath.c_str(), layername, &err);
    if (err) {
        fprintf(stderr, "EXR error = %s\n", err);
    }
    if (status != 0) {
        fprintf(stderr, "Load EXR err: %s\n", err);
        std::cout << " LoadEXRRGBA() FAILED \n";
    }
    textureData.m_data = floatPtr;
    return textureData;
}

TextureData LoadTextureData(std::string filepath) {
    stbi_set_flip_vertically_on_load(false);
    TextureData textureData;
    textureData.m_data = stbi_load(filepath.data(), &textureData.m_width, &textureData.m_height, &textureData.m_numChannels, 0);
    return textureData;

}

TextureData LoadDDSTextureData(std::string filepath) {
    TextureData textureData;
    return textureData;
}

void SaveDDSFile(std::string filename, void* data, int width, int height, int numChannels) {
    /*
    std::string suffix = filename.substr(filename.length() - 3);

    if (suffix == "NRM" || suffix == "ALB" || suffix == "RMA") {
        uint8_t* imageData = static_cast<uint8_t*>(data);
        if (numChannels == 3) {
            const uint64_t pitch = static_cast<uint64_t>(width) * 3UL;
            for (auto r = 0; r < height; ++r) {
                uint8_t* row = imageData + r * pitch;
                for (auto c = 0UL; c < static_cast<uint64_t>(width); ++c) {
                    uint8_t* pixel = row + c * 3UL;
                    uint8_t  p = pixel[0];
                    pixel[0] = pixel[2];
                    pixel[2] = p;
                }
            }
        }

        CMP_Texture cmpTexture;

        if (suffix == "NRM") {
            //swizzle
            CMP_Texture srcTexture = { 0 };
            srcTexture.dwSize = sizeof(CMP_Texture);
            srcTexture.dwWidth = width;
            srcTexture.dwHeight = height;
            srcTexture.dwPitch = numChannels == 4 ? width * 4 : width * 3;
            srcTexture.format = numChannels == 4 ? CMP_FORMAT_RGBA_8888 : CMP_FORMAT_RGB_888;
            srcTexture.dwDataSize = srcTexture.dwHeight * srcTexture.dwPitch;
            srcTexture.pData = imageData;

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
            CMP_Texture srcTexture = { 0 };
            srcTexture.dwSize = sizeof(CMP_Texture);
            srcTexture.dwWidth = _width;
            srcTexture.dwHeight = _height;
            srcTexture.dwPitch = _NumOfChannels == 4 ? _width * 4 : _width * 3;
            srcTexture.format = _NumOfChannels == 4 ? CMP_FORMAT_RGBA_8888 : CMP_FORMAT_BGR_888;
            srcTexture.dwDataSize = srcTexture.dwHeight * srcTexture.dwPitch;
            srcTexture.pData = imageData;
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
            CMP_Texture srcTexture = { 0 };
            srcTexture.dwSize = sizeof(CMP_Texture);
            srcTexture.dwWidth = _width;
            srcTexture.dwHeight = _height;
            srcTexture.dwPitch = _NumOfChannels == 4 ? _width * 4 : _width * 3;
            srcTexture.format = _NumOfChannels == 4 ? CMP_FORMAT_RGBA_8888 : CMP_FORMAT_RGB_888;
            srcTexture.dwDataSize = srcTexture.dwHeight * srcTexture.dwPitch;
            srcTexture.pData = imageData;
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
    }*/
}

bool OpenGLTexture::Load(const std::string filepath) {

    if (!Util::FileExists(filepath)) {
        std::cout << filepath << " does not exist.\n";
        return false;
    }
    _filename = Util::GetFilename(filepath);
    _filetype = Util::GetFileInfo(filepath).filetype;

    // Is it a VAT texture?
    if (_filetype == "exr") {
        TextureData textureData = LoadEXRData(filepath);
        this->m_data = textureData.m_data;
        this->_width = textureData.m_width;
        this->_height = textureData.m_height;
        return true;
    }

    // Check if compressed version exists. If not, create one.
    std::string suffix = _filename.substr(_filename.length() - 3);
    std::string compressedPath = "res/assets/" + _filename + ".dds";

    if (!Util::FileExists(compressedPath)) {

        TextureData textureData = LoadTextureData(filepath);
        this->m_data = textureData.m_data;
        this->_width = textureData.m_width;
        this->_height = textureData.m_height;
        this->_NumOfChannels = textureData.m_numChannels;
    }

    /*
    if (Util::FileExists(compressedPath)) {
        std::cout << compressedPath << "\n";
        //LoadDDSFile(compressedPath.c_str(), m_cmpTexture);
        m_compressedTextureData = LoadCompressedDDSFromDisk(compressedPath.c_str());
        m_compressed = true;
    }*/

    if (!m_compressed) {
    //if (_CMP_texture == nullptr) {
        //For everything else just load the raw texture. Compression fucks up UI elements.
        stbi_set_flip_vertically_on_load(false);
        m_data = stbi_load(filepath.data(), &_width, &_height, &_NumOfChannels, 0);
    }
    return true;
}


GLenum MapGliFormatToOpenGL(gli::format format) {
    switch (format) {
    case gli::FORMAT_RGBA_DXT1_UNORM_BLOCK8:
    case gli::FORMAT_RGBA_DXT1_SRGB_BLOCK8:
        return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
    case gli::FORMAT_RGBA_DXT3_UNORM_BLOCK16:
    case gli::FORMAT_RGBA_DXT3_SRGB_BLOCK16:
        return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
    case gli::FORMAT_RGBA_DXT5_UNORM_BLOCK16:
    case gli::FORMAT_RGBA_DXT5_SRGB_BLOCK16:
        return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        // Add more cases for other formats if needed
    default:
        std::cerr << "Unsupported format!" << std::endl;
        return 0;
    }
}

const char* GetGLFormatString(GLenum format) {
    switch (format) {
    case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
        return "GL_COMPRESSED_RGB_S3TC_DXT1_EXT";
    case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        return "GL_COMPRESSED_RGBA_S3TC_DXT1_EXT";
    case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
        return "GL_COMPRESSED_RGBA_S3TC_DXT3_EXT";
    case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
        return "GL_COMPRESSED_RGBA_S3TC_DXT5_EXT";
    case GL_COMPRESSED_RED_RGTC1:
        return "GL_COMPRESSED_RED_RGTC1";
    case GL_COMPRESSED_SIGNED_RED_RGTC1:
        return "GL_COMPRESSED_SIGNED_RED_RGTC1";
    case GL_COMPRESSED_RG_RGTC2:
        return "GL_COMPRESSED_RG_RGTC2";
    case GL_COMPRESSED_SIGNED_RG_RGTC2:
        return "GL_COMPRESSED_SIGNED_RG_RGTC2";
    case GL_COMPRESSED_RGB8_ETC2:
        return "GL_COMPRESSED_RGB8_ETC2";
    case GL_COMPRESSED_SRGB8_ETC2:
        return "GL_COMPRESSED_SRGB8_ETC2";
    case GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:
        return "GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2";
    case GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2:
        return "GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2";
    case GL_COMPRESSED_RGBA8_ETC2_EAC:
        return "GL_COMPRESSED_RGBA8_ETC2_EAC";
    case GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC:
        return "GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC";
        // Add more formats as needed
    default:
        return "Unknown Format";
    }
}

bool OpenGLTexture::Bake() {

    if (_baked) {
        return true;
    }

    _baked = true;

    if (m_compressed) {

        std::cout << _filename << ": " << GetGLFormatString(m_compressedTextureData.format) << "\n";
        std::cout << " -width: " << std::to_string(m_compressedTextureData.width) << "\n";
        std::cout << " -height: " << std::to_string(m_compressedTextureData.height) << "\n";
        std::cout << " -size: " << std::to_string(m_compressedTextureData.size) << "\n";
        std::cout << " -data: " << m_compressedTextureData.data << "\n";


        glGenTextures(1, &ID);
        glBindTexture(GL_TEXTURE_2D, ID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glCompressedTexImage2D(GL_TEXTURE_2D, 0, m_compressedTextureData.format, m_compressedTextureData.width, m_compressedTextureData.height, 0, m_compressedTextureData.size, m_compressedTextureData.data);

        glGenerateMipmap(GL_TEXTURE_2D);

        bindlessID = glGetTextureHandleARB(ID);
        glMakeTextureHandleResidentARB(bindlessID);

        return true;

    }

    if (m_data == nullptr && _floatData == nullptr) {


        if (_filetype == "exr") {
            std::cout << "exr bake failed coz some pointer was null\n";
        }

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
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16, _width, _height, 0, GL_RGBA, GL_FLOAT, m_data);
        std::cout << "baked exr texture\n";
        free(m_data);
    }
    else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _width, _height, 0, format, GL_UNSIGNED_BYTE, m_data);
        stbi_image_free(m_data);
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

    m_data = nullptr;
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
