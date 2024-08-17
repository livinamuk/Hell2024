#pragma once

#pragma warning(push, 0)
#define GLM_FORCE_SILENT_WARNINGS
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "glm/gtx/hash.hpp"
#include "Math.h"
#include <gli/gli.hpp>
#pragma warning(pop)

enum class API { OPENGL, VULKAN, UNDEFINED };
enum class WindowedMode { WINDOWED, FULLSCREEN };
enum class SplitscreenMode { NONE, TWO_PLAYER, FOUR_PLAYER, SPLITSCREEN_MODE_COUNT };
enum class BulletHoleDecalType { REGULAR, GLASS };
enum class PickUpType { NONE, AMMO, GLOCK, GLOCK_AMMO, TOKAREV_AMMO, SHOTGUN, SHOTGUN_AMMO, AKS74U, AKS74U_AMMO, AKS74U_SCOPE };
enum class DobermannState { LAY, PATROL, KAMAKAZI, DOG_SHAPED_PIECE_OF_MEAT };
enum class FacingDirection { LEFT, RIGHT, ALIGNED };

enum EngineMode { GAME = 0, FLOORPLAN, EDITOR };
enum Weapon { KNIFE = 0, GLOCK, SHOTGUN, AKS74U, MP7, WEAPON_COUNT };
enum WeaponAction {
    IDLE = 0,
    FIRE,
    RELOAD,
    RELOAD_FROM_EMPTY,
    DRAW_BEGIN,
    DRAWING,
    SPAWNING,
    RELOAD_SHOTGUN_BEGIN,
    RELOAD_SHOTGUN_SINGLE_SHELL,
    RELOAD_SHOTGUN_DOUBLE_SHELL,
    RELOAD_SHOTGUN_END,
    ADS_IN,
    ADS_OUT,
    ADS_IDLE,
    ADS_FIRE,
    MELEE
};

#define _propogationGridSpacing 0.375f
#define _pointCloudSpacing 0.4f
#define _maxPropogationDistance 2.6f


#define PLAYER_COUNT 4
#define UNDEFINED_STRING "UNDEFINED_STRING"

#define AUDIO_SELECT "SELECT_2.wav"
#define ENV_MAP_SIZE 2048

#define DOOR_VOLUME 1.0f
#define INTERACT_DISTANCE 2.5f

#define NEAR_PLANE 0.005f
//#define FAR_PLANE 50.0f
#define FAR_PLANE 500.0f

#define NOOSE_PI 3.14159265359f
#define NOOSE_HALF_PI 1.57079632679f
#define HELL_PI 3.141592653589793f

#define DOOR_WIDTH 0.8f
#define DOOR_HEIGHT 2.0f
#define DOOR_EDITOR_DEPTH 0.05f

#define WINDOW_WIDTH 0.85f
#define WINDOW_HEIGHT 2.1f

#define PLAYER_CAPSULE_HEIGHT 0.4f
#define PLAYER_CAPSULE_RADIUS 0.15f

//#define MAP_WIDTH   32
//#define MAP_HEIGHT  16
//#define MAP_DEPTH   50

#define ZERO_MEM(a) memset(a, 0, sizeof(a))
#define ARRAY_SIZE_IN_ELEMENTS(a) (sizeof(a)/sizeof(a[0]))
#define SAFE_DELETE(p) if (p) { delete p; p = NULL; }
#define ToRadian(x) (float)(((x) * HELL_PI / 180.0f))
#define ToDegree(x) (float)(((x) * 180.0f / HELL_PI))

#define ORANGE   glm::vec3(1, 0.647f, 0)
#define BLACK   glm::vec3(0,0,0)
#define WHITE   glm::vec3(1,1,1)
#define RED     glm::vec3(1,0,0)
#define GREEN   glm::vec3(0,1,0)
#define BLUE    glm::vec3(0,0,1)
#define YELLOW  glm::vec3(1,1,0)
#define PURPLE  glm::vec3(1,0,1)
#define GREY    glm::vec3(0.25f)
#define LIGHT_BLUE    glm::vec3(0,1,1)
#define LIGHT_GREEN   glm::vec3(0.16f, 0.78f, 0.23f)
#define LIGHT_RED     glm::vec3(0.8f, 0.05f, 0.05f)
#define GRID_COLOR    glm::vec3(0.509, 0.333, 0.490) * 0.5f

#define SMALL_NUMBER		(float)9.99999993922529e-9
#define KINDA_SMALL_NUMBER	(float)0.00001
#define MIN_RAY_DIST        (float)0.01f

#define NRM_X_FORWARD glm::vec3(1,0,0)
#define NRM_X_BACK glm::vec3(-1,0,0)
#define NRM_Y_UP glm::vec3(0,1,0)
#define NRM_Y_DOWN glm::vec3(0,-1,0)
#define NRM_Z_FORWARD glm::vec3(0,0,1)
#define NRM_Z_BACK glm::vec3(0,0,-1)

#define POSITION_LOCATION    0
#define NORMAL_LOCATION		 1
#define TEX_COORD_LOCATION   2
#define TANGENT_LOCATION     3
#define BONE_ID_LOCATION     4
#define BONE_WEIGHT_LOCATION 5

namespace hell {
    struct ivec2 {
        int x;
        int y;
        ivec2() = default;
        template <typename T>
        ivec2(T x_, T y_) : x(static_cast<int>(x_)), y(static_cast<int>(y_)) {}
        ivec2(const ivec2& other_) : x(other_.x), y(other_.y) {}
        ivec2(int x_, int y_) : x(x_), y(y_) {}
        ivec2 operator+(const ivec2& other) const {
            return ivec2(x + other.x, y + other.y);
        }
        ivec2 operator-(const ivec2& other) const {
            return ivec2(x - other.x, y - other.y);
        }
        ivec2& operator=(const ivec2& other) {
            if (this != &other) {
                x = other.x;
                y = other.y;
            }
            return *this;
        }
    };
}

enum VB_TYPES {
    INDEX_BUFFER,
    POS_VB,
    NORMAL_VB,
    TEXCOORD_VB,
    TANGENT_VB,
    BITANGENT_VB,
    BONE_VB,
    SMOOTH_NORMAL_VB,
    NUM_VBs
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

/*struct Triangle {
    glm::vec3 p1 = glm::vec3(0);
    glm::vec3 p2 = glm::vec3(0);
    glm::vec3 p3 = glm::vec3(0);
    glm::vec3 normal = glm::vec3(0);
    glm::vec3 color = glm::vec3(0);
};*/



struct GridProbe {
    glm::vec3 color = BLACK;
    //int samplesRecieved = 0;
    bool ignore = true; // either blocked by geometry, or out of map range
};

struct RenderItem2DB {
    std::string textureName;
    int screenX = 0;
    int screenY = 0;
    glm::mat4 modelMatrix = glm::mat4(1);
    glm::vec3 color = WHITE;
    bool centered = false;
    GLuint target = GL_TEXTURE_2D;
    void* parent = nullptr;
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

//enum class RigidStaticType { NONE, FLOOR, WALLS, ENEMY, DOOR };



struct FileInfo {
    std::string fullpath;
    std::string directory;
    std::string filename;
    std::string filetype;
    std::string materialType;
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

enum RaycastGroup {
    RAYCAST_DISABLED = 0,
    RAYCAST_ENABLED = 1,
    PLAYER_1_RAGDOLL = 2,
    PLAYER_2_RAGDOLL = 4,
    PLAYER_3_RAGDOLL = 8,
    PLAYER_4_RAGDOLL = 16,
    DOBERMAN = 32
};

enum class PhysicsObjectType {
    UNDEFINED,
    GAME_OBJECT,
    GLASS,
    DOOR,
    WINDOW,
    SCENE_MESH,
    RAGDOLL_RIGID,
    CSG_OBJECT_ADDITIVE,
    CSG_OBJECT_SUBTRACTIVE,
    LIGHT,
};

struct PhysicsObjectData {
    PhysicsObjectData(PhysicsObjectType type, void* parent) {
        this->type = type;
        this->parent = parent;
    }
    PhysicsObjectType type;
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
    PhysicsObjectType physicsObjectType;
};

enum CollisionGroup {
    NO_COLLISION = 0,
    BULLET_CASING = 1,
    PLAYER = 2,
    ENVIROMENT_OBSTACLE = 4,
    GENERIC_BOUNCEABLE = 8,
    ITEM_PICK_UP = 16,
    RAGDOLL = 32,
    DOG_CHARACTER_CONTROLLER = 64,
};

/*struct AABB2 {
    glm::vec3 position = glm::vec3(0);
    glm::vec3 extents = glm::vec3(0);
};*/


// RAY TRACING SHIT

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

public: // make private later
    glm::vec3 center = glm::vec3(0);
    glm::vec3 boundsMin = glm::vec3(1e30f);
    glm::vec3 boundsMax = glm::vec3(-1e30f);

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
