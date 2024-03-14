#include "ExrTexture.h"
#define TINYEXR_IMPLEMENTATION
#include "tinyexr.h"

ExrTexture::ExrTexture()
{
}

ExrTexture::ExrTexture(std::string filepath)
{
    const char* filename = filepath.c_str();
    bool ret;

    ret = GetEXRLayers(filename);
    if (!ret)
        std::cout << " GetEXRLayers() FAILED \n";
        
    ret = LoadEXRRGBA(&gExrRGBA, &gExrWidth, &gExrHeight, filename, layername);
        if (!ret)
            std::cout << " LoadEXRRGBA() FAILED \n";

    /*std::cout << " gExrRGBA: " << *gExrRGBA << "\n";
    std::cout << " gExrWidth: " << gExrWidth << "\n";
    std::cout << " gExrHeight: " << gExrHeight << "\n";*/
    //std::cout << " layername: " << *layername << "\n";


    // Load to GL
    gTexId;
    glGenTextures(1, &gTexId);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTexId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // @todo { Use GL_RGBA32F for internal texture format. }
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, gExrWidth, gExrHeight, 0, GL_RGBA, GL_FLOAT, gExrRGBA);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16, gExrWidth, gExrHeight, 0, GL_RGBA, GL_FLOAT, gExrRGBA);

    /*  for (int i = 0; i < gExrWidth; i++) {
        std::cout << i << ": " << gExrRGBA[i] << "\n";
    }*/
}

bool ExrTexture::GetEXRLayers(const char* filename)
{
    const char** layer_names = nullptr;
    int num_layers = 0;
    const char* err = nullptr;
    int ret = EXRLayers(filename, &layer_names, &num_layers, &err);

    //std::cout << " num_layers: " << num_layers << "\n";

    if (err) {
        fprintf(stderr, "EXR error = %s\n", err);
    }

    if (ret != 0) {
        fprintf(stderr, "Load EXR err: %s\n", err);
        return false;
    }
    if (num_layers > 0)
    {
        fprintf(stdout, "EXR Contains %i Layers\n", num_layers);
        for (int i = 0; i < (int)num_layers; ++i) {
            fprintf(stdout, "Layer %i : %s\n", i + 1, layer_names[i]);
        }
    }
    free(layer_names);
    return true;
}

bool ExrTexture::LoadEXRRGBA(float** rgba, int* w, int* h, const char* filename, const char* layername)
{
    int width, height;
    float* image;
    const char* err = nullptr;
    int ret = LoadEXRWithLayer(&image, &width, &height, filename, layername, &err);

    if (err) {
        fprintf(stderr, "EXR error = %s\n", err);
    }

    if (ret != 0) {
        fprintf(stderr, "Load EXR err: %s\n", err);
        return false;
    }

    (*rgba) = image;
    (*w) = width;
    (*h) = height;

    return true;
}
