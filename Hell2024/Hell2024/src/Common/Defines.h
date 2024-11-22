#pragma once
#include <glm/glm.hpp>

constexpr static auto TL = 0;
constexpr static auto TR = 1;
constexpr static auto BL = 2;
constexpr static auto BR = 3;

constexpr static auto _propogationGridSpacing = 0.375f;
constexpr static auto _pointCloudSpacing = 0.4f;
constexpr static auto _maxPropogationDistance = 2.6f;

constexpr static auto PLAYER_COUNT = 4;
constexpr static auto UNDEFINED_STRING = "UNDEFINED_STRING";

constexpr static auto AUDIO_SELECT = "SELECT_2.wav";

constexpr static auto DOOR_VOLUME = 1.0f;
constexpr static auto INTERACT_DISTANCE = 2.5f;

constexpr static auto NEAR_PLANE = 0.0025f;
constexpr static auto FAR_PLANE = 200.0f;

constexpr static auto NOOSE_PI = 3.14159265359f;
constexpr static auto NOOSE_HALF_PI = 1.57079632679f;
constexpr static auto HELL_PI = 3.141592653589793f;
constexpr static auto HELL_PHI = 1.6180f;

constexpr static auto DOOR_WIDTH = 0.8f;
constexpr static auto DOOR_HEIGHT = 2.0f;
constexpr static auto DOOR_EDITOR_DEPTH = 0.05f;

constexpr static auto WINDOW_WIDTH = 0.85f;
constexpr static auto WINDOW_HEIGHT = 2.1f;

constexpr static auto PLAYER_CAPSULE_HEIGHT = 0.4f;
constexpr static auto PLAYER_CAPSULE_RADIUS = 0.15f;

constexpr static auto CSG_PLANE_CUBE_HACKY_OFFSET = 0.1f;

#define ZERO_MEM(a) memset(a, 0, sizeof(a))
#define ARRAY_SIZE_IN_ELEMENTS(a) (sizeof(a)/sizeof(a[0]))
#define SAFE_DELETE(p) if (p) { delete p; p = NULL; }
#define ToRadian(x) (float)(((x) * HELL_PI / 180.0f))
#define ToDegree(x) (float)(((x) * 180.0f / HELL_PI))

constexpr static auto ORANGE = glm::vec3(1, 0.647f, 0);
constexpr static auto BLACK = glm::vec3(0, 0, 0);
constexpr static auto WHITE = glm::vec3(1, 1, 1);
constexpr static auto RED = glm::vec3(1, 0, 0);
constexpr static auto GREEN = glm::vec3(0, 1, 0);
constexpr static auto BLUE = glm::vec3(0, 0, 1);
constexpr static auto YELLOW = glm::vec3(1, 1, 0);
constexpr static auto PURPLE = glm::vec3(1, 0, 1);
constexpr static auto GREY = glm::vec3(0.25f);
constexpr static auto LIGHT_BLUE = glm::vec3(0, 1, 1);
constexpr static auto LIGHT_GREEN = glm::vec3(0.16f, 0.78f, 0.23f);
constexpr static auto LIGHT_RED = glm::vec3(0.8f, 0.05f, 0.05f);
constexpr static auto GRID_COLOR = glm::vec3(0.509, 0.333, 0.490) * 0.5f;

constexpr static auto SMALL_NUMBER= (float)9.99999993922529e-9;
constexpr static auto KINDA_SMALL_NUMBER = (float)0.00001;
constexpr static auto MIN_RAY_DIST = (float)0.01f;

constexpr static auto NRM_X_FORWARD = glm::vec3(1, 0, 0);
constexpr static auto NRM_X_BACK = glm::vec3(-1, 0, 0);
constexpr static auto NRM_Y_UP = glm::vec3(0, 1, 0);
constexpr static auto NRM_Y_DOWN = glm::vec3(0, -1, 0);
constexpr static auto NRM_Z_FORWARD = glm::vec3(0, 0, 1);
constexpr static auto NRM_Z_BACK = glm::vec3(0, 0, -1);

constexpr static auto SHADOW_MAP_SIZE = 1024;
constexpr static auto SHADOW_NEAR_PLANE = 0.05f;
constexpr static auto DEFAULT_LIGHT_COLOR = glm::vec3(1, 0.7799999713897705, 0.5289999842643738);
constexpr static auto LIGHT_VOLUME_AABB_COLOR_MAP_SIZE = 256;
