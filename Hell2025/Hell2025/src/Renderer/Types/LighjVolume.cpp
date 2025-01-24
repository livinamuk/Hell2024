#include "LightVolume.h"
#include "../../BackEnd/BackEnd.h"
#include "RendererCommon.h"

LightVolume::LightVolume(float width, float height, float depth, float posX, float posY, float posZ) {

    this->width = width;
    this->height = height;
    this->depth = depth;
    this->posX = posX;
    this->posY = posY;
    this->posZ = posZ;


    int xCount = width / PROBE_SPACING;
    int yCount = height / PROBE_SPACING;
    int zCount = depth / PROBE_SPACING;

  /*std::cout << "width: " << width << "\n";
    std::cout << "height: " << height << "\n";
    std::cout << "depth: " << depth << "\n\n";

    std::cout << "xCount: " << xCount << "\n";
    std::cout << "yCount: " << yCount << "\n";
    std::cout << "zCount: " << zCount << "\n\n";

    std::cout << width * height * depth << "\n";
    std::cout << xCount * yCount * zCount << "\n\n";*/
}

void LightVolume::CreateTexure3D() {

    int textureWidth = width / PROBE_SPACING;
    int textureHeight = height / PROBE_SPACING;
    int textureDepth = depth / PROBE_SPACING;

    //std::cout << "Created 3D texture of size " << textureWidth << " x " << textureHeight << " x " << textureDepth << "\n";

    if (BackEnd::GetAPI() == API::OPENGL) {
        texutre3D.GetGLTexture3D().Create(textureWidth, textureHeight, textureDepth);
    }
    if (BackEnd::GetAPI() == API::VULKAN) {
        texutre3D.GetVKTexture3D().Create(textureWidth, textureHeight, textureDepth);
    }
}

const float LightVolume::GetWorldSpaceWidth() {
    return width;
}

const float LightVolume::GetWorldSpaceHeight() {
    return height;
}

const float LightVolume::GetWorldSpaceDepth() {
    return depth;
}

const int LightVolume::GetProbeSpaceWidth() {
    return  width / PROBE_SPACING;
}

const int LightVolume::GetProbeSpaceHeight() {
    return height / PROBE_SPACING;;
}

const int LightVolume::GetProbeSpaceDepth() {
    return depth / PROBE_SPACING;;
}

const int LightVolume::GetProbeCount() {
    int xCount = width / PROBE_SPACING;
    int yCount = height / PROBE_SPACING;
    int zCount = depth / PROBE_SPACING;
    return xCount * yCount * zCount;
}

const glm::vec3 LightVolume::GetPosition() {
    return glm::vec3(posX, posY, posZ);
}

const void LightVolume::CleanUp() {
    texutre3D.CleanUp();
}