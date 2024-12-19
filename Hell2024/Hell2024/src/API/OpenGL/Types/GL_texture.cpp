#include <iostream>
#include "GL_texture.h"
#include "../../../Util.hpp"
#include "../../vendor/DDS/DDS_Helpers.h"
#include <stb_image.h>
#include "Tools/ImageTools.h"

bool dontDoBindless = false;

GLuint64 OpenGLTexture::GetBindlessID() {
    return bindlessID;
}

uint32_t CmpToOpenGlFormat(CMP_FORMAT format) {
    //std::cout << "FORMAT IS: " << format << "\n";
    switch (format) {
    case CMP_FORMAT_DXT1:
        return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
    case CMP_FORMAT_DXT3:
        return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
    case CMP_FORMAT_DXT5:
        return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
    case CMP_FORMAT_BC4:
        return GL_COMPRESSED_RED_RGTC1; // Single-channel compressed format
    case CMP_FORMAT_BC5:
        return GL_COMPRESSED_RG_RGTC2;  // Two-channel compressed format
    case CMP_FORMAT_ATI2N_XY:
        return GL_COMPRESSED_RG_RGTC2;  // Two-channel compressed format
    case CMP_FORMAT_BC6H:
        return GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT; // HDR format
    case CMP_FORMAT_BC7:
        return GL_COMPRESSED_RGBA_BPTC_UNORM;         // High-quality RGBA
    case CMP_FORMAT_ETC2_RGB:
        return GL_COMPRESSED_RGB8_ETC2;
    case CMP_FORMAT_ETC2_SRGB:
        return GL_COMPRESSED_SRGB8_ETC2;
    case CMP_FORMAT_ETC2_RGBA:
        return GL_COMPRESSED_RGBA8_ETC2_EAC;
    case CMP_FORMAT_ETC2_SRGBA:
        return GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC;
    case CMP_FORMAT_ASTC:
        return GL_COMPRESSED_RGBA_ASTC_4x4_KHR; // Assuming ASTC 4x4 block size
    case CMP_FORMAT_BC1:
        return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
    case CMP_FORMAT_BC2:
        return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
    default:
        return 0xFFFFFFFF; // Invalid format
    }
}

void freeCMPTexture(CMP_Texture* t) {
    free(t->pData);
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

    if (textureData.m_data == nullptr) {
        std::cerr << "Failed to load texture: " << filepath << "\n";
        std::cerr << "STB Image Error: " << stbi_failure_reason() << "\n";
        textureData.m_width = 0;
        textureData.m_height = 0;
        textureData.m_numChannels = 0;
    }

    return textureData;
}

TextureData LoadDDSTextureData(std::string filepath) {
    TextureData textureData;
    return textureData;
}

bool OpenGLTexture::Load(const std::string filepath, bool compressed) {

    if (!Util::FileExists(filepath)) {
        std::cout << filepath << " does not exist.\n";
        return false;
    }
    m_filename = Util::GetFilename(filepath);
    m_filetype = Util::GetFileInfo(filepath).filetype;

    // Is it a VAT texture?
    if (m_filetype == "exr") {
        TextureData textureData = LoadEXRData(filepath);
        this->m_data = textureData.m_data;
        this->_width = textureData.m_width;
        this->_height = textureData.m_height;
        return true;
    }

    TextureData textureData = LoadTextureData(filepath);
    this->m_data = textureData.m_data;
    this->_width = textureData.m_width;
    this->_height = textureData.m_height;
    this->_NumOfChannels = textureData.m_numChannels;

    return true;
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

    if (m_compressed) {

        std::cout << m_filename << ": " << GetGLFormatString(m_compressedTextureData.format) << "\n";
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
        if (!dontDoBindless) {
            bindlessID = glGetTextureHandleARB(ID);
            glMakeTextureHandleResidentARB(bindlessID);
        }

        return true;
    }

    if (m_data == nullptr) {
        std::cout << "ATTENTION! OpenGLTexture::Bake() called but m_data was nullptr:" << m_filename << "\n";
        return false;
    }

    glGenTextures(1, &ID);
    glBindTexture(GL_TEXTURE_2D, ID);

    GLint format = GL_RGB;
    if (_NumOfChannels == 4)
        format = GL_RGBA;
    if (_NumOfChannels == 1)
        format = GL_RED;


    if (m_filetype == "exr") {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16, _width, _height, 0, GL_RGBA, GL_FLOAT, m_data);
        //std::cout << "baked exr texture\n";
        free(m_data);
        m_data = nullptr;
    }
    else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
       // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _width, _height, 0, format, GL_UNSIGNED_BYTE, m_data);
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(m_data);
        m_data = nullptr;
    }

    // Hack to make Resident Evil font look okay when scaled
    std::string filename = this->GetFilename();
    if (filename.substr(0, 4) == "char") {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    }
    else {
        glGenerateMipmap(GL_TEXTURE_2D);
    }


    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


    if (!dontDoBindless) {
        bindlessID = glGetTextureHandleARB(ID);
        glMakeTextureHandleResidentARB(bindlessID);
    }
    //glMakeTextureHandleNonResidentARB(bindlessID); to unbind

    m_data = nullptr;
    return true;
}


void OpenGLTexture::BakeCMPData(CMP_Texture* cmpTexture) {
    uint32_t glFormat = CmpToOpenGlFormat(cmpTexture->format);
    if (glFormat == 0xFFFFFFFF) {
        std::cout << "Invalid format! Failed to load compressed texture: " << m_filename << "\n";
        return;
    }
    glGenTextures(1, &ID);
    glBindTexture(GL_TEXTURE_2D, ID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    uint32_t width = cmpTexture->dwWidth;
    uint32_t height = cmpTexture->dwHeight;
    uint32_t size = cmpTexture->dwDataSize;
    glCompressedTexImage2D(GL_TEXTURE_2D, 0, glFormat, cmpTexture->dwWidth, cmpTexture->dwHeight, 0, size, cmpTexture->pData);
    glGenerateMipmap(GL_TEXTURE_2D);
    freeCMPTexture(cmpTexture);
    if (!dontDoBindless) {
        bindlessID = glGetTextureHandleARB(ID);
        glMakeTextureHandleResidentARB(bindlessID);
    }
}

void OpenGLTexture::HotloadFromPath(const std::string filepath) {

    m_compressed = false;
    TextureData textureData = LoadTextureData(filepath);

    if (!textureData.m_data) {
        std::cout << "OpenGLTexture::HotloadFromPath() failed to load texture data for: " << filepath << "\n";
        return;
    }
    else {
        std::cout << "Hotloaded texture:" << filepath << "\n";
    }

    this->m_data = textureData.m_data;
    this->_width = textureData.m_width;
    this->_height = textureData.m_height;
    this->_NumOfChannels = textureData.m_numChannels;

    GLint format = GL_RGB;
    if (_NumOfChannels == 4) {
        format = GL_RGBA;
    }
    if (_NumOfChannels == 1) {
        format = GL_RED;
    }

    // Unbind bindless
    glMakeTextureHandleNonResidentARB(bindlessID);

    glDeleteTextures(1, &ID);
    glGenTextures(1, &ID);
    glBindTexture(GL_TEXTURE_2D, ID);

    // Upload the new texture data
    glTexImage2D(GL_TEXTURE_2D, 0, format, _width, _height, 0, format, GL_UNSIGNED_BYTE, m_data);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Clean up texture data after upload
    stbi_image_free(m_data);
    m_data = nullptr;

    // Update the texture handle and make it resident again
    bindlessID = glGetTextureHandleARB(ID);
    glMakeTextureHandleResidentARB(bindlessID);

    // Unbind texture to avoid unintentional state changes
    glBindTexture(GL_TEXTURE_2D, 0);
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
    return m_filename;
}

std::string& OpenGLTexture::GetFiletype() {
    return m_filetype;
}

/*
bool OpenGLTexture::IsBaked() {
    return _baked;
}
*/