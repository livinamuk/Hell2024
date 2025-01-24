#include "ExrTexture.h"
#define TINYEXR_IMPLEMENTATION
#include "tinyexr.h"

void ExrTexture::Load(std::string filepath) {

    int pos = filepath.rfind("\\") + 1;
    int pos2 = filepath.rfind("/") + 1;
    _filename = filepath.substr(max(pos, pos2));
    _filename = _filename.substr(0, _filename.length() - 4);
    _filetype = filepath.substr(filepath.length() - 3);

    const char* filename = filepath.c_str();

    // Get EXR Layers
    const char* err = nullptr;
    const char** layer_names = nullptr;
    const char* layername = nullptr;
    int num_layers = 0;
    bool status = EXRLayers(filename, &layer_names, &num_layers, &err);
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
    float* image;
    status = LoadEXRWithLayer(&image, &width, &height, filename, layername, &err);
    if (err) {
        fprintf(stderr, "EXR error = %s\n", err);
    }
    if (status != 0) {
        fprintf(stderr, "Load EXR err: %s\n", err);
        std::cout << " LoadEXRRGBA() FAILED \n";
    }
    gExrRGBA = image;
    gExrWidth = width;
    gExrHeight = height;



    // Load to GL
    gTexId;
    glGenTextures(1, &gTexId);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTexId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16, gExrWidth, gExrHeight, 0, GL_RGBA, GL_FLOAT, gExrRGBA);
}

std::string& ExrTexture::GetFilename() {
    /*if (BackEnd::GetAPI() == API::OPENGL) {
        return glTexture.GetFilename();
    }
    else if (BackEnd::GetAPI() == API::VULKAN) {
        return vkTexture.GetFilename();
    }*/
    return _filename;
}

std::string& ExrTexture::GetFiletype() {
    return _filetype;
    /*
    if (BackEnd::GetAPI() == API::OPENGL) {
        return glTexture.GetFiletype();
    }
    else if (BackEnd::GetAPI() == API::VULKAN) {
        return vkTexture.GetFiletype();
    }*/
}

OpenGLTexture& ExrTexture::GetGLTexture() {
    return glTexture;
}

VulkanTexture& ExrTexture::GetVKTexture() {
    return vkTexture;
}