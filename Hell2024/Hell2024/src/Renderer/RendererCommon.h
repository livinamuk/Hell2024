#pragma once

#define GLM_FORCE_SILENT_WARNINGS
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "glm/gtx/hash.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define PRESENT_WIDTH 832
#define PRESENT_HEIGHT 468

#define TEXTURE_ARRAY_SIZE 1024
#define MAX_RENDER_OBJECTS_3D 4096
#define MAX_RENDER_OBJECTS_2D 4096
#define MAX_TLAS_OBJECT_COUNT 4096
#define FRAME_OVERLAP 2
#define MAX_LIGHTS 32
#define MAX_ANIMATED_TRANSFORMS 2048
#define MAX_INSTANCES 4096
#define MAX_INDIRECT_COMMANDS 4096
#define MAX_GLASS_MESH_COUNT 128
#define MAX_DECAL_COUNT 4096
#define MAX_BLOOD_DECAL_COUNT 1024
#define MAX_VAT_INSTANCE_COUNT 16

enum Alignment { 
    CENTERED,
    TOP_LEFT,
    TOP_RIGHT,
    BOTTOM_LEFT,
    BOTTOM_RIGHT
};


struct DrawIndexedIndirectCommand {
    uint32_t indexCount;
    uint32_t instanceCount;
    uint32_t firstIndex;
    int32_t  baseVertex;
    uint32_t baseInstance;
};

struct GPULight {
    float posX;
    float posY;
    float posZ;
    float colorR;
    float colorG;
    float colorB;
    float strength;
    float radius;
};

struct RenderItem2D {
    glm::mat4 modelMatrix = glm::mat4(1);
    float colorTintR;
    float colorTintG;
    float colorTintB;
    int textureIndex;
}; 

struct InstanceData {
    glm::mat4 modelMatrix;
    glm::mat4 inverseModelMatrix;
    int baseColorTextureIndex;
    int normalTextureIndex;
    int rmaTextureIndex;
    int useEmissiveMask = 0; // 0 for nothing, 1 for emissive output 
    glm::vec3 emissiveColor;
    int padding0;
};

struct RenderItem3D {
    glm::mat4 modelMatrix = glm::mat4(1);
    glm::mat4 inverseModelMatrix = glm::mat4(1);
    int meshIndex;
    int baseColorTextureIndex;
    int normalTextureIndex;
    int rmaTextureIndex;
    int vertexOffset;
    int indexOffset;
    int animatedTransformsOffset; 
    int castShadow = 1;             // if 0 then currently also it is not included in the TLAS
    int useEmissiveMask = 0;
    glm::vec3 emissiveColor = glm::vec3(0);

    // Overloading < operator for sorting with std::sort
    bool operator<(const RenderItem3D& obj) const {
        return meshIndex < obj.meshIndex;
    }
};

struct RenderItem3DInstanced {
    int meshIndex;
    int baseColorTextureIndex;
    int normalTextureIndex;
    int rmaTextureIndex;
    int vertexOffset;
    int indexOffset;
    int instanceCount;
    int modelMatrixOffset;
};


struct CameraData {
    glm::mat4 projection = glm::mat4(1);
    glm::mat4 projectionInverse = glm::mat4(1);
    glm::mat4 view = glm::mat4(1);
    glm::mat4 viewInverse = glm::mat4(1);
    float viewportWidth = 0;
    float viewportHeight = 0;
    float padding0 = 0;
    float padding1 = 0;
};

struct BoundingBox {
    glm::vec3 size;
    glm::vec3 offsetFromModelOrigin;
};

struct Vertex {

    Vertex() = default;
    Vertex(glm::vec3 pos) {
        position = pos;
    }
    Vertex(glm::vec3 pos, glm::vec3 norm) {
        position = pos;
        normal = norm;
    }
    glm::vec3 position = glm::vec3(0);
    glm::vec3 normal = glm::vec3(0);
    glm::vec2 uv = glm::vec2(0);
    glm::vec3 tangent = glm::vec3(0);

    bool operator==(const Vertex& other) const {
        return position == other.position && normal == other.normal && uv == other.uv;
    }
};

struct WeightedVertex {
    glm::vec3 position = glm::vec3(0);
    glm::vec3 normal = glm::vec3(0);
    glm::vec2 uv = glm::vec2(0);
    glm::vec3 tangent = glm::vec3(0);
    glm::ivec4 boneID = glm::ivec4(0);
    glm::vec4 weight = glm::vec4(0);

    bool operator==(const Vertex& other) const {
        return position == other.position && normal == other.normal && uv == other.uv;
    }
};

struct DebugVertex {
    glm::vec3 position = glm::vec3(0);
    glm::vec3 color = glm::vec3(0);
};

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.position) ^ (hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.uv) << 1);
        }
    };
}

struct CloudPointOld {
    glm::vec4 position = glm::vec4(0);
    glm::vec4 normal = glm::vec4(0);
    glm::vec4 directLighting = glm::vec4(0);
};

enum DebugLineRenderMode {
    SHOW_NO_LINES,
    PHYSX_ALL,
    PHYSX_RAYCAST,
    PHYSX_COLLISION,
    RAYTRACE_LAND,
    PHYSX_EDITOR,
    BOUNDING_BOXES,
    DEBUG_LINE_MODE_COUNT
};

enum RenderMode {
    COMPOSITE, 
    DIRECT_LIGHT, 
    INDIRECT_LIGHT, 
    POINT_CLOUD,
    RENDER_MODE_COUNT
};