#pragma once
#include <string>
#include "../API/OPenGL/Types/GL_types.h"
#include "../API/Vulkan/Types/VK_types.h"
#include "../Physics/Physics.h"

struct VulkanMeshData {
    AllocatedBuffer vertexBuffer;
    AllocatedBuffer indexBuffer;
    AllocatedBuffer transformBuffer;
    AccelerationStructure accelerationStructure;
};

struct OpenGLMeshData {
    //GLuint VBO = 0;
    //GLuint VAO = 0;
    //GLuint EBO = 0;
};

struct Mesh {

    std::string name = "undefined";

    uint32_t baseVertex = 0;
    uint32_t baseIndex = 0;
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;

    VulkanMeshData vulkanMeshData = {};
    OpenGLMeshData openGLMeshData = {};

    PxTriangleMesh* triangleMesh = NULL;
    PxConvexMesh* convexMesh = NULL;

    bool uploadedToGPU = false;
};

