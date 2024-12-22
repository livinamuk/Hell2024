#pragma once
#pragma warning(push, 0)
#define GLM_FORCE_SILENT_WARNINGS
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "glm/gtx/hash.hpp"
//#include "Math.h"
#include <gli/gli.hpp>
#pragma warning(pop)
#include "Defines.h"
#include "Enums.h"


struct TriangleIntersectionResult {
    bool hitFound = false;
    glm::vec3 hitPosition = glm::vec3(0);
};

struct PlayerData {
    int flashlightOn = 0;
    int padding0 = 0;
    int padding1 = 0;
    int padding2 = 0;
};

struct AssetFile {
    char type[4];
    int version;
    std::string json;
    std::vector<char> binaryBlob;
};

struct Transform {
    glm::vec3 position = glm::vec3(0);
    glm::vec3 rotation = glm::vec3(0);
    glm::vec3 scale = glm::vec3(1);
    glm::mat4 to_mat4() {
        glm::mat4 m = glm::translate(glm::mat4(1), position);
        m *= glm::mat4_cast(glm::quat(rotation));
        m = glm::scale(m, scale);
        return m;
    };
    glm::vec3 to_forward_vector() {
        glm::quat q = glm::quat(rotation);
        return glm::normalize(q * glm::vec3(0.0f, 0.0f, 1.0f));
    }
    glm::vec3 to_right_vector() {
        glm::quat q = glm::quat(rotation);
        return glm::normalize(q * glm::vec3(1.0f, 0.0f, 0.0f));
    }
};

struct Point {
    glm::vec3 pos = { glm::vec3(0) };
    glm::vec3 color = { glm::vec3(0) };
    Point() {};
    Point(glm::vec3 pos, glm::vec3 color) {
        this->pos = pos;
        this->color = color;
    }
    Point(float x, float y, float z, glm::vec3 color) {
        this->pos = glm::vec3(x, y, z);
        this->color = color;
    }
};

struct Line {
    Point p1;
    Point p2;
    Line() {};
    Line(glm::vec3 start, glm::vec3 end, glm::vec3 color) {
        p1.pos = start;
        p2.pos = end;
        p1.color = color;
        p2.color = color;
    }
    glm::vec3 GetCenter() {
        return (p1.pos + p2.pos) * 0.5f;
    }
};



struct GridProbe {
    glm::vec3 color = BLACK;
    //int samplesRecieved = 0;
    bool ignore = true; // either blocked by geometry, or out of map range
};


struct UIRenderInfo {
    std::string textureName;
    int screenX = 0;
    int screenY = 0;
    glm::mat4 modelMatrix = glm::mat4(1);
    glm::vec3 color = WHITE;
    bool centered = false;
    GLuint target = GL_TEXTURE_2D;
    void* parent = nullptr;
};

struct FileInfoOLD {
    std::string fullpath;
    std::string directory;
    std::string filename;
    std::string filetype;
    std::string materialType;
};

struct FileInfo {
    std::string path;
    std::string name;
    std::string ext;
    std::string dir;
    std::string GetFileNameWithExtension() {
        if (ext.length() > 0) {
            return name + "." + ext;
        }
        else {
            return name;
        }
    }
};


struct Material {
    Material() {}
    std::string _name = UNDEFINED_STRING;
    int _basecolor = 0;
    int _normal = 0;
    int _rma = 0;
};

struct GPUMaterial {
    int basecolor = 0;
    int normal = 0;
    int rma = 0;
    int padding = 0;
};

struct PhysicsObjectData {
    PhysicsObjectData(ObjectType type, void* parent) {
        this->type = type;
        this->parent = parent;
    }
    ObjectType type;
    void* parent;
};

struct PhysXRayResult {
    std::string hitObjectName;
    glm::vec3 hitPosition;
    glm::vec3 surfaceNormal;
    glm::vec3 rayDirection;
    bool hitFound;
    void* hitActor;
    void* parent;
    ObjectType objectType;
};

struct OverlapResult {
    ObjectType objectType;
    glm::vec3 position;
    void* parent;
};

struct EditorVertex {
    glm::vec3 position;
    void* parent;
    ObjectType objectType;
};

struct Sphere {
    glm::vec3 origin = glm::vec3(0);
    float radius = 0.0f;
};

struct AABB {
    AABB() {}
    AABB(glm::vec3 min, glm::vec3 max) {
        boundsMin = min;
        boundsMax = max;
        CalculateCenter();
    }
    void Grow(AABB& b) {
        if (b.boundsMin.x != 1e30f && b.boundsMin.x != -1e30f) {
            Grow(b.boundsMin); Grow(b.boundsMax);
        }
        CalculateCenter();
    }
    void Grow(glm::vec3 p) {
        boundsMin = glm::vec3(std::min(boundsMin.x, p.x), std::min(boundsMin.y, p.y), std::min(boundsMin.z, p.z));
        boundsMax = glm::vec3(std::max(boundsMax.x, p.x), std::max(boundsMax.y, p.y), std::max(boundsMax.z, p.z));
        CalculateCenter();
    }
    float Area() {
        glm::vec3 e = boundsMax - boundsMin; // box extent
        return e.x * e.y + e.y * e.z + e.z * e.x;
    }
    const glm::vec3 GetCenter() {
        return center;
    }
    const glm::vec3 GetBoundsMin() {
        return boundsMin;
    }
    const glm::vec3 GetBoundsMax() {
        return boundsMax;
    }
    const glm::vec3 GetExtents() const {
        return (boundsMax - boundsMin) / 2.0f;
    }
    bool ContainsPoint(const glm::vec3& point) const {
        return (point.x >= boundsMin.x && point.x <= boundsMax.x &&
            point.y >= boundsMin.y && point.y <= boundsMax.y &&
            point.z >= boundsMin.z && point.z <= boundsMax.z);
    }
    bool ContainsSphere(const glm::vec3& sphereCenter, const float sphereRadius) const {
        glm::vec3 closestPoint = glm::clamp(sphereCenter, boundsMin, boundsMax);
        float distanceSquared = glm::dot(closestPoint - sphereCenter, closestPoint - sphereCenter);
        return distanceSquared <= (sphereRadius * sphereRadius);
        
      //float distanceSquared = glm::distance(center, sphereCenter);
      //
      //bool res = distanceSquared <= (sphereRadius * sphereRadius);
      //std::cout << "Failed sphere test for " << Util::Vec3ToString(boundsMin, )
      //
      //return distanceSquared <= (sphereRadius * sphereRadius);
    }

public: // make private later
    glm::vec3 center = glm::vec3(0);
    glm::vec3 boundsMin = glm::vec3(1e30f);
    glm::vec3 boundsMax = glm::vec3(-1e30f);
    glm::vec3 padding = glm::vec3(0);

    void CalculateCenter() {
        center = { (boundsMin.x + boundsMax.x) / 2, (boundsMin.y + boundsMax.y) / 2, (boundsMin.z + boundsMax.z) / 2 };
    }
};

struct BLASInstance {
    glm::mat4 inverseModelMatrix = glm::mat4(1);
    int blsaRootNodeIndex = 0;
    int baseTriangleIndex = 0;
    int baseVertex = 0;
    int baseIndex = 0;
};

struct Triangle {
    glm::vec3 v0 = glm::vec3(0);
    glm::vec3 v1 = glm::vec3(0);
    glm::vec3 v2 = glm::vec3(0);
    glm::vec3 centoid = glm::vec3(0);
    glm::vec3 aabbMin = glm::vec3(0);
    glm::vec3 aabbMax = glm::vec3(0);
};


struct BVHNode {
    glm::vec3 aabbMin; int leftFirst = -1;
    glm::vec3 aabbMax; int instanceCount = -1;
    bool IsLeaf() { return instanceCount > 0; }
};

struct TextureData {
    std::string m_filepath;
    int m_width = 0;
    int m_height = 0;
    int m_numChannels = 0;
    void* m_data = nullptr;
};

struct CompressedTextureData {
    int width = 0;
    int height = 0;
    int size = 0;
    void* data = nullptr;
    gli::target target = gli::TARGET_2D;
    GLenum format = 0;
};
