#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <Compressonator.h>
#include <string>
#include <memory>

struct OpenGLTexture {

    OpenGLTexture() = default;
    explicit OpenGLTexture(const std::string filepath);
    GLuint GetID();
    GLuint64 GetBindlessID();
    void Bind(unsigned int slot);
    bool Load(const std::string filepath);
    bool Bake();
    void UploadToGPU(void* data, CMP_Texture* cmpTexture, int width, int height, int channelCount);
    bool IsBaked();
    int GetWidth();
    int GetHeight();
    std::string& GetFilename();
    std::string& GetFiletype();

private:
    GLuint ID;
    GLuint64 bindlessID;
    std::string _filename;
    std::string _filetype;
    std::unique_ptr<CMP_Texture> _CMP_texture;
    unsigned char* _data = nullptr;
    float* _floatData = nullptr;
    int _NumOfChannels = 0;
    int _width = 0;
    int _height = 0;
    bool _baked = false;
};
