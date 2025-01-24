#pragma once
#include <glm/glm.hpp>

#define TL 0
#define TR 1
#define BL 2
#define BR 3

#define _propogationGridSpacing 0.275f
#define _pointCloudSpacing 0.4f
#define _maxPropogationDistance 2.6f

#define PLAYER_COUNT 4
#define UNDEFINED_STRING "UNDEFINED_STRING"

#define AUDIO_SELECT "SELECT_2.wav"

#define DOOR_VOLUME 1.0f
#define INTERACT_DISTANCE 2.5f

#define NEAR_PLANE 0.0025f
#define FAR_PLANE 200.0f

#define NOOSE_PI 3.14159265359f
#define NOOSE_HALF_PI 1.57079632679f
#define HELL_PI 3.141592653589793f
#define HELL_PHI 1.6180f

#define DOOR_WIDTH 0.8f
#define DOOR_HEIGHT 2.0f
#define DOOR_EDITOR_DEPTH 0.05f

#define WINDOW_WIDTH 0.85f
#define WINDOW_HEIGHT 2.1f

#define PLAYER_CAPSULE_HEIGHT 0.4f
#define PLAYER_CAPSULE_RADIUS 0.15f

#define CSG_PLANE_CUBE_HACKY_OFFSET 0.1f

#define ZERO_MEM(a) memset(a, 0, sizeof(a))
#define ARRAY_SIZE_IN_ELEMENTS(a) (sizeof(a)/sizeof(a[0]))
#define SAFE_DELETE(p) if (p) { delete p; p = NULL; }
#define ToRadian(x) (float)(((x) * HELL_PI / 180.0f))
#define ToDegree(x) (float)(((x) * 180.0f / HELL_PI))

#define ORANGE  glm::vec3(1, 0.647f, 0)
#define BLACK   glm::vec3(0, 0, 0)
#define WHITE   glm::vec3(1, 1, 1)
#define RED     glm::vec3(1, 0, 0)
#define GREEN   glm::vec3(0, 1, 0)
#define BLUE    glm::vec3(0, 0, 1)
#define YELLOW  glm::vec3(1, 1, 0)
#define PURPLE  glm::vec3(1, 0, 1)
#define GREY    glm::vec3(0.25f)
#define LIGHT_BLUE    glm::vec3(0, 1, 1)
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

#define SHADOW_MAP_SIZE 1024
#define SHADOW_NEAR_PLANE 0.05f
#define DEFAULT_LIGHT_COLOR glm::vec3(1, 0.7799999713897705, 0.5289999842643738)
#define LIGHT_VOLUME_AABB_COLOR_MAP_SIZE 256
