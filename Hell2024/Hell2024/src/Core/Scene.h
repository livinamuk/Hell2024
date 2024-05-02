#pragma once
#include "../Common.h"
#include "GameObject.h"
#include "Window.h"
#include "Player.h"
#include "../Physics/Physics.h"
#include "../Effects/BulletCasing.h"
#include "../Effects/Decal.h"
#include "Light.h"
#include "VolumetricBloodSplatter.h"
#include "../Util.hpp"

#include "../Types/Modular/Door.h"
#include "../Types/Modular/Ceiling.h"
#include "../Types/Modular/Floor.h"
#include "../Types/Modular/Toilet.hpp"
#include "../Types/Modular/Wall.h"

struct BloodDecal {
    Transform transform;
    Transform localOffset;
    int type;
    glm::mat4 modelMatrix;

    BloodDecal(Transform transform, int type) {
        this->transform = transform;
        this->type = type;
        if (type != 2) {
            localOffset.position.z = 0.55f;
            transform.scale = glm::vec3(2.0f);
        }
        else {
            localOffset.rotation.y = Util::RandomFloat(0, HELL_PI * 2);;
            transform.scale = glm::vec3(1.5f);
        }
        modelMatrix = transform.to_mat4() * localOffset.to_mat4();
    }
};




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
    PxU32 raycastFlags;
    glm::vec3 parentPlayersViewRotation;
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


    void Update(float deltaTime);




    // OLD SHIT BELOW
    inline PxTriangleMesh* _sceneTriangleMesh = NULL;
    inline PxRigidStatic* _sceneRigidDynamic = NULL;
    inline PxShape* _sceneShape = NULL;

    inline std::vector<Toilet> _toilets;
    inline std::vector<BloodDecal> _bloodDecals;
    inline std::vector<VolumetricBloodSplatter> _volumetricBloodSplatters;
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

    // New shit
    void LoadMapNEW(std::string mapPath);
    std::vector<RenderItem3D> GetAllRenderItems();

    // Old shit
	void LoadHardCodedObjects();
	void LoadMap(std::string mapPath);
	void SaveMap(std::string mapPath);
    void CleanUp();
    void Update_OLD(float deltaTime);
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
    void CheckForDirtyLights();
    void ResetGameObjectStates();

    void CreateVolumetricBlood(glm::vec3 position, glm::vec3 rotation, glm::vec3 front);

    //Player* GetPlayerFromCharacterControler(PxController* characterController);
}