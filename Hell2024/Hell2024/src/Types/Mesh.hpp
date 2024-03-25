#pragma once
#include <string>
#include "../API/Vulkan/Types/VK_raytracing.hpp"
#include "../Physics/Physics.h"

struct Mesh {

    std::string name = "undefined";

    int32_t baseVertex = 0;
    uint32_t baseIndex = 0;
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;

    PxTriangleMesh* triangleMesh = NULL;    // TO DO
    PxConvexMesh* convexMesh = NULL;        // TO DO

    AccelerationStructure accelerationStructure;

    bool uploadedToGPU = false;
};

