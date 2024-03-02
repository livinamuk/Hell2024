#pragma once

#pragma warning(push, 0)
#define GLM_FORCE_SILENT_WARNINGS
#define GLM_ENABLE_EXPERIMENTAL
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "glm/gtx/hash.hpp"
#include "Math.h"
#pragma warning(pop)

enum EngineMode { GAME = 0, FLOORPLAN, EDITOR };
enum ViewportMode {FULLSCREEN = 0, SPLITSCREEN, VIEWPORTMODE_COUNT};
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
    ADS_FIRE
};

#define AUDIO_SELECT "SELECT.wav"
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

#define PLAYER_CAPSULE_HEIGHT 0.5f
#define PLAYER_CAPSULE_RADIUS 0.1f

//#define MAP_WIDTH   32
//#define MAP_HEIGHT  16
//#define MAP_DEPTH   50

#define ZERO_MEM(a) memset(a, 0, sizeof(a))
#define ARRAY_SIZE_IN_ELEMENTS(a) (sizeof(a)/sizeof(a[0]))
#define SAFE_DELETE(p) if (p) { delete p; p = NULL; }
#define ToRadian(x) (float)(((x) * HELL_PI / 180.0f))
#define ToDegree(x) (float)(((x) * 180.0f / HELL_PI)) 

#define BLACK   glm::vec3(0,0,0)
#define WHITE   glm::vec3(1,1,1)
#define RED     glm::vec3(1,0,0)
#define GREEN   glm::vec3(0,1,0)
#define BLUE    glm::vec3(0,0,1)
#define YELLOW  glm::vec3(1,1,0)
#define PURPLE  glm::vec3(1,0,1)
#define GREY    glm::vec3(0.25f)
#define LIGHT_BLUE    glm::vec3(0,1,1)
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
#define BITANGENT_LOCATION   4
#define BONE_ID_LOCATION     5
#define BONE_WEIGHT_LOCATION 6
#define SMOOTH_NORMAL_LOCATION 7

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

struct Vertex {
	glm::vec3 position = glm::vec3(0);
	glm::vec3 normal = glm::vec3(0);
    glm::vec2 uv = glm::vec2(0);
    glm::vec3 tangent = glm::vec3(0);
    glm::vec3 bitangent = glm::vec3(0);
    glm::vec4 weight = glm::vec4(0);
    glm::ivec4 boneID = glm::ivec4(0);

    bool operator==(const Vertex& other) const {
        return position == other.position && normal == other.normal && uv == other.uv;
    }
};

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.position) ^ (hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.uv) << 1);
        }
    };
}

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
};

struct VoxelFace {

    int x = 0;
    int y = 0;
    int z = 0;
    glm::vec3 baseColor;
    glm::vec3 normal;
    glm::vec3 accumulatedDirectLighting = { glm::vec3(0) };
    glm::vec3 indirectLighting = { glm::vec3(0) };

    VoxelFace(int x, int y, int z, glm::vec3 baseColor, glm::vec3 normal) {
        this->x = x;
        this->y = y;
        this->z = z;
        this->baseColor = baseColor;
        this->normal = normal;
    }
};




struct Triangle {
    glm::vec3 p1 = glm::vec3(0);
    glm::vec3 p2 = glm::vec3(0);
    glm::vec3 p3 = glm::vec3(0);
    glm::vec3 normal = glm::vec3(0);
    glm::vec3 color = glm::vec3(0);
};

struct IntersectionResult {
    bool found = false;
    float distance = 0;
    float dot = 0;
    glm::vec2 baryPosition = glm::vec2(0);
};

struct GridProbe {
    glm::vec3 color = BLACK;
    //int samplesRecieved = 0;
    bool ignore = true; // either blocked by geometry, or out of map range
};

struct Extent2Di {
    int width;
    int height;
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

//enum class RaycastObjectType { NONE, FLOOR, WALLS, ENEMY, DOOR };



struct FileInfo {
    std::string fullpath;
    std::string directory;
    std::string filename;
    std::string filetype;
    std::string materialType;
};

struct Material {
    Material() {}
    std::string _name = "undefined";
    int _basecolor = 0;
    int _normal = 0;
    int _rma = 0;
};

enum RaycastGroup { RAYCAST_DISABLED = 0, RAYCAST_ENABLED };
enum PhysicsObjectType { UNDEFINED = 0, GAME_OBJECT, GLASS, DOOR, SCENE_MESH, RAGDOLL_RIGID};

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
};

struct AABB {
    glm::vec3 position = glm::vec3(0);
    glm::vec3 extents = glm::vec3(0);
};