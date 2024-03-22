#pragma once
#include <glad/glad.h>
#include <Compressonator.h>
#include <string>
#include <memory>

struct OpenGLTexture {

    OpenGLTexture() = default;
    explicit OpenGLTexture(const std::string_view filepath);
    GLuint GetID();
    void Bind(unsigned int slot);
    bool Load(const std::string_view filepath);
    bool Bake();
    bool IsBaked();

    int GetWidth();
    int GetHeight();
    std::string& GetFilename();
    std::string& GetFiletype();

private:
    GLuint ID;
    std::string _filename;
    std::string _filetype;
    std::unique_ptr<CMP_Texture> _CMP_texture;
    unsigned char* _data = nullptr;
    int _NumOfChannels = 0;
    int _width = 0;
    int _height = 0;
    bool _baked = false;
};
