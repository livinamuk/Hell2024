#pragma once
#include "HellCommon.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <Compressonator.h>
#include <string>
#include <memory>

struct OpenGLTexture {

    OpenGLTexture() = default;
    //explicit OpenGLTexture(const std::string filepath);
    GLuint GetID();
    GLuint64 GetBindlessID();
    void Bind(unsigned int slot);
    bool Load(const std::string filepath, bool compressed);
    bool Bake();
    void BakeCMPData(CMP_Texture* cmpTexture);
    //void UploadToGPU(void* data, CMP_Texture* cmpTexture, int width, int height, int channelCount);
    bool IsBaked();
    int GetWidth();
    int GetHeight();
    std::string& GetFilename();
    std::string& GetFiletype();
    void HotloadFromPath(const std::string filepath);
    void MakeBindlessTextureResident();
    void MakeBindlessTextureNonResident();

    GLuint& GetHandleReference();
    void GenerateMipmaps();

private:
    GLuint ID;
    GLuint64 bindlessID = 0;
    std::string m_fullPath;
    std::string m_filename;
    std::string m_filetype;
    bool m_compressed = false;

    //unsigned char* _data = nullptr;
    //float* _floatData = nullptr;

    CompressedTextureData m_compressedTextureData;

    void* m_data = nullptr;

    int _NumOfChannels = 0;
    int _width = 0;
    int _height = 0;
    //bool _baked = false;
};
