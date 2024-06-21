#pragma once
#include "GL_cubemapTexture.h"
#include <stb_image.h>
#include <glad/glad.h>
#include <vector>
#include <iostream>

void OpenGLCubemapTexture::LoadAndBake(std::string name, std::string filetype) {

    std::vector<std::string> filepaths;
    filepaths.push_back("res/textures/skybox/" + name + "_Right." + filetype);
    filepaths.push_back("res/textures/skybox/" + name + "_Left." + filetype);
    filepaths.push_back("res/textures/skybox/" + name + "_Top." + filetype);
    filepaths.push_back("res/textures/skybox/" + name + "_Bottom." + filetype);
    filepaths.push_back("res/textures/skybox/" + name + "_Front." + filetype);
    filepaths.push_back("res/textures/skybox/" + name + "_Back." + filetype);

    for (auto& filepath : filepaths) {
        std::cout << filepath << "\n";
    }

    glGenTextures(1, &ID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, ID);

    int numOfChannels;
    for (unsigned int i = 0; i < 6; i++) {
        unsigned char* data = stbi_load(filepaths[i].c_str(), &width, &height, &numOfChannels, 0);
        if (data) {
            GLint format = GL_RGB;
            if (numOfChannels == 4)
                format = GL_RGBA;
            if (numOfChannels == 1)
                format = GL_RED;
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else {
            std::cout << "Failed to load cubemap\n";
            stbi_image_free(data);
        }
    }
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

}

void OpenGLCubemapTexture::Bind(unsigned int slot) {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_CUBE_MAP, ID);
}

unsigned int OpenGLCubemapTexture::GetID() {
    return ID;
}

unsigned int OpenGLCubemapTexture::GetWidth() {
    return width;
}

unsigned int OpenGLCubemapTexture::GetHeight() {
    return height;
}
