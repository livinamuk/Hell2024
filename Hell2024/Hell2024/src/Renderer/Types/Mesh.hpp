#pragma once
#include <glm/glm.hpp>
#include <string>
#include "../../API/Vulkan/Types/VK_raytracing.hpp"

struct Mesh {

    std::string name = "undefined";

    int32_t baseVertex = 0;
    uint32_t baseIndex = 0;
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;

    glm::vec3 aabbMin;
    glm::vec3 aabbMax;

    AccelerationStructure accelerationStructure;

    bool uploadedToGPU = false;
};

