#pragma once
#include "../Common.h"
#include "GameObject.h"
#include "AnimatedGameObject.h"
#include "Door.h"
#include "Window.h"
#include "Player.h"
#include "Physics.h"
#include "../Effects/BulletCasing.h"
#include "../Effects/Decal.h"
#include "Light.h"
#include "Floor.h"

#define WALL_HEIGHT 2.4f

inline float RandFloat(float min, float max) {
    return min + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (max - min)));
}

inline glm::vec3 NormalFromThreePoints(glm::vec3 pos0, glm::vec3 pos1, glm::vec3 pos2) {
    return glm::normalize(glm::cross(pos1 - pos0, pos2 - pos0));
}

inline void SetNormalsAndTangentsFromVertices(Vertex* vert0, Vertex* vert1, Vertex* vert2) {
    // Shortcuts for UVs
    glm::vec3& v0 = vert0->position;
    glm::vec3& v1 = vert1->position;
    glm::vec3& v2 = vert2->position;
    glm::vec2& uv0 = vert0->uv;
    glm::vec2& uv1 = vert1->uv;
    glm::vec2& uv2 = vert2->uv;
    // Edges of the triangle : position delta. UV delta
    glm::vec3 deltaPos1 = v1 - v0;
    glm::vec3 deltaPos2 = v2 - v0;
    glm::vec2 deltaUV1 = uv1 - uv0;
    glm::vec2 deltaUV2 = uv2 - uv0;
    float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
    glm::vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
    glm::vec3 bitangent = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r;
    glm::vec3 normal = NormalFromThreePoints(vert0->position, vert1->position, vert2->position);
    vert0->normal = normal;
    vert1->normal = normal;
    vert2->normal = normal;
    vert0->tangent = tangent;
    vert1->tangent = tangent;
    vert2->tangent = tangent;
    vert0->bitangent = bitangent;
    vert1->bitangent = bitangent;
    vert2->bitangent = bitangent;
}

#define PROPOGATION_SPACING 1
#define PROPOGATION_WIDTH (MAP_WIDTH / PROPOGATION_SPACING)
#define PROPOGATION_HEIGHT (MAP_HEIGHT / PROPOGATION_SPACING)
#define PROPOGATION_DEPTH (MAP_DEPTH / PROPOGATION_SPACING)

struct CloudPoint {
    glm::vec4 position = glm::vec4(0);
    glm::vec4 normal = glm::vec4(0);
    glm::vec4 directLighting = glm::vec4(0);
};


struct SpawnPoint {
    glm::vec3 position = glm::vec3(0);
    glm::vec3 rotation = glm::vec3(0);
};


struct Wall {
    glm::vec3 begin = glm::vec3(0);
    glm::vec3 end;
    float height = 0;
    GLuint VAO = 0;
    GLuint VBO = 0;
    std::vector<Vertex> vertices;
    int materialIndex = 0;
    float wallHeight = 2.4f;
    void CreateMesh();
    Wall(glm::vec3 begin, glm::vec3 end, float height, int materialIndex);
    glm::vec3 GetNormal();
    glm::vec3 GetMidPoint();
    void Draw();
    std::vector<Transform> ceilingTrims;
    std::vector<Transform> floorTrims;
    std::vector<Line> collisionLines;
};


struct Ceiling {
    float x1, z1, x2, z2, height;
    GLuint VAO = 0;
    GLuint VBO = 0;
    std::vector<Vertex> vertices;
    int materialIndex = 0;
    Ceiling(float x1, float z1, float x2, float z2, float height, int materialIndex);
    void Draw();
};

struct RTMesh {
    GLuint baseVertex = 0;
    GLuint vertexCount = 0;
    GLuint padding0 = 0;
    GLuint padding1 = 0;
};

struct RTInstance {
    glm::mat4 modelMatrix = glm::mat4(1);
    glm::mat4 inverseModelMatrix = glm::mat4(1);
    GLuint meshIndex = 0;
    GLuint padding0 = 0;
    GLuint padding1 = 0;
    GLuint padding2 = 0;
};

struct Bullet {
    glm::vec3 spawnPosition;
    glm::vec3 direction;
    Weapon type;
};


struct PickUp {

	enum class Type { GLOCK_AMMO = 0};

	Type type;
	glm::vec3 position;
	glm::vec3 rotation;
	std::string parentGameObjectName = "";
	bool pickedUp = false;
    float timeSincePickedUp = 0.0f;
	float respawnTime = 10.0f;

	glm::mat4 GetModelMatrix() {
		Transform transform;
		transform.position = position;
		transform.rotation = rotation;
		return transform.to_mat4();
	}

    void Update(float deltaTime) {

        if (pickedUp) {
            timeSincePickedUp += deltaTime;
        }
        if (timeSincePickedUp > respawnTime) {
            pickedUp = false;
            timeSincePickedUp = 0;
        }
    }
};

namespace Scene {

    inline PxTriangleMesh* _sceneTriangleMesh = NULL;
    inline PxRigidStatic* _sceneRigidDynamic = NULL;
    inline PxShape* _sceneShape = NULL;

    inline std::vector<PickUp> _pickUps;
    inline std::vector<SpawnPoint> _spawnPoints;
    inline std::vector<Bullet> _bullets;
    inline std::vector<BulletCasing> _bulletCasings;
    inline std::vector<Decal> _decals;
	inline std::vector<Wall> _walls;
	inline std::vector<Window> _windows;
	inline std::vector<Door> _doors;
    inline std::vector<Floor> _floors;
    inline std::vector<Ceiling> _ceilings;
    inline std::vector<CloudPoint> _cloudPoints;
    inline std::vector<GameObject> _gameObjects;
    inline std::vector<AnimatedGameObject> _animatedGameObjects;
    inline std::vector<Light> _lights;
    inline std::vector<glm::vec3> _rtVertices;
    inline std::vector<RTMesh> _rtMesh;
    inline std::vector<RTInstance> _rtInstances;
    inline std::vector<Player> _players;

	void LoadHardCodedObjects();
	void LoadMap(std::string mapPath);
	void SaveMap(std::string mapPath);
    void CreatePlayers();
   // void NewScene();
    void CleanUp();
    void Update(float deltaTime);
    void Update3DEditorScene();
    void LoadLightSetup(int index);
    GameObject* GetGameObjectByName(std::string);
    AnimatedGameObject* GetAnimatedGameObjectByName(std::string);
    std::vector<AnimatedGameObject>& GetAnimatedGameObjects();
    void CreatePointCloud();
    void CreateMeshData();
    void AddLight(Light& light);
    void AddDoor(Door& door);
    void AddWall(Wall& wall);
    void AddFloor(Floor& floor);
    void UpdateRTInstanceData();
    void RecreateDataStructures();
    //void CreateScenePhysicsObjects();
    void ProcessPhysicsCollisions();
	void RecreateAllPhysicsObjects();
	void RemoveAllDecalsFromWindow(Window* window); 
    void CalculateLightBoundingVolumes();
    void CheckIfLightsAreDirty();
    //Player* GetPlayerFromCharacterControler(PxController* characterController);
}