#pragma once
#include "RendererCommon.h"
#include "../Core/Audio.h"
#include "../Physics/RigidBody.hpp"
#include "../Physics/RigidStatic.hpp"
#include "../Physics/Physics.h"
#include "../Renderer/Types/Model.hpp"
#include "../Util.hpp"

enum class OpenState { NONE, CLOSED, CLOSING, OPEN, OPENING };
enum class OpenAxis { NONE, TRANSLATE_X, TRANSLATE_Y, TRANSLATE_Z, ROTATION_POS_X, ROTATION_POS_Y, ROTATION_POS_Z, ROTATION_NEG_X, ROTATION_NEG_Y, ROTATION_NEG_Z };
enum class InteractType { NONE, TEXT, QUESTION, PICKUP, CALLBACK_ONLY };
enum class ModelMatrixMode { GAME_TRANSFORM, PHYSX_TRANSFORM };
enum class CollisionType { NONE, STATIC_ENVIROMENT, STATIC_ENVIROMENT_NO_DOG, BOUNCEABLE, PICKUP, BULLET_CASING };

struct GameObject {

public:
    CollisionType m_collisionType = CollisionType::NONE;

public:
    GameObject() = default;
    void SetCollisionType(CollisionType collisionType);
    void DisableShadows();

    RigidBody m_collisionRigidBody;
    RigidStatic m_raycastRigidStatic;
    std::vector<BlendingMode> m_meshBlendingModes;
    std::vector<int> m_meshMaterialIndices;






public:
    Model* model = nullptr;
	Transform _transform;
	Transform _openTransform;
	std::string _name = "undefined";
	std::string _parentName = "undefined";
	OpenState _openState = OpenState::NONE;
	OpenAxis _openAxis = OpenAxis::NONE;
	float _maxOpenAmount = 0;
	float _minOpenAmount = 0;
	float _openSpeed = 0;
    bool _respawns = true;
    bool m_castShadows = true;

    void MakeGold();

private:

	PickUpType _pickupType = PickUpType::NONE;
	bool _collected = false;
    float _pickupCoolDownTime = 0;
    bool m_isGold;

	ModelMatrixMode _modelMatrixMode = ModelMatrixMode::GAME_TRANSFORM;
	BoundingBox _boundingBox;

	struct AudioEffects {
		AudioEffectInfo onOpen;
		AudioEffectInfo onClose;
		AudioEffectInfo onInteract;
	} _audio;

private:
	glm::mat4 _modelMatrix;


public:
	glm::mat4 GetModelMatrix();
	std::string GetName();

	glm::vec3 GetWorldPosition();
	glm::quat GetWorldRotation();
	void UpdateEditorPhysicsObject();

	glm::vec3 GetWorldSpaceOABBCenter();
    void SetKinematic(bool value);
    void DisableRaycasting();
    void EnableRaycasting();

    AABB _aabb;
    AABB _aabbPreviousFrame;

	//void SetModelScaleWhenUsingPhysXTransform(glm::vec3 scale);

	glm::vec3 _scaleWhenUsingPhysXTransform = glm::vec3(1);

	void SetOpenAxis(OpenAxis openAxis);
	void SetAudioOnInteract(const char* filename, float volume);
	void SetAudioOnOpen(const char* filename, float volume);
	void SetAudioOnClose(const char* filename, float volume);
	//void SetInteract(InteractType type, std::string text, std::function<void(void)> callback);
    void SetOpenState(OpenState openState, float speed, float min, float max);
    void SetPosition(glm::vec3 position);
    void SetRotation(glm::vec3 rotation);
	void SetPositionX(float position);
	void SetPositionY(float position);
	void SetPositionZ(float position);
	void SetPosition(float x, float y, float z);
	void SetRotationX(float rotation);
	void SetRotationY(float rotation);
	void SetRotationZ(float rotation);
	float GetRotationX();
	float GetRotationY();
	float GetRotationZ();
	void SetScale(glm::vec3 scale);
	void SetScale(float scale);
	void SetScaleX(float scale);
	void SetName(std::string name);
	void SetParentName(std::string name);
	std::string GetParentName();
	bool IsInteractable();
	void Interact();
	void Update(float deltaTime);
    void SetModel(const std::string& name);
    void SetMeshMaterial(const char* name, int meshIndex = -1);
    void SetMeshBlendingMode(const char* meshName, BlendingMode blendingMode);
    void SetMeshBlendingModes(BlendingMode blendingMode);
	void SetCollectedState(bool value);
	BoundingBox GetBoundingBox();
	const InteractType& GetInteractType();
	OpenState& GetOpenState();
	void SetTransform(Transform& transform);
	void SetMeshMaterialByMeshName(std::string meshName, const char* materialName);
	void PickUp();
	void SetPickUpType(PickUpType pickupType);
	bool IsCollectable();
	bool IsCollected();
	PickUpType GetPickUpType();
	//void CreateEditorPhysicsObject();

    int m_convexModelIndex = -1;

	//void CreateRigidBody(glm::mat4 matrix, bool kinematic);
	void AddCollisionShape(PxShape* shape, PhysicsFilterData physicsFilterData);
	void AddCollisionShapeFromModelIndex(unsigned int modelIndex, glm::vec3 scale = glm::vec3(1));
	void AddCollisionShapeFromBoundingBox(BoundingBox& boundignBox);
	void UpdateRigidBodyMassAndInertia(float density);
    void PutRigidBodyToSleep();

	//void SetRaycastShapeFromMesh(OpenGLMesh* mesh);
	void SetRaycastShapeFromModelIndex(unsigned int modelIndex);
	void SetRaycastShape(PxShape* shape);

	void SetModelMatrixMode(ModelMatrixMode modelMatrixMode);
	void SetPhysicsTransform(glm::mat4 worldMatrix);
	glm::mat4 GetGameWorldMatrix(); // aka not the physx matrix
	void AddForceToCollisionObject(glm::vec3 direction, float strength);

    void CleanUp();
    bool HasMovedSinceLastFrame();

    void LoadSavedState();
    void SetWakeOnStart(bool value);
    void UpdateRigidStatic();
    void DisableRespawnOnPickup();
    void UpdatePhysXPointers();

    std::vector<RenderItem3D>& GetRenderItems();
    std::vector<RenderItem3D>& GetRenderItemsBlended();
    std::vector<RenderItem3D>& GetRenderItemsAlphaDiscarded();

    std::vector<Triangle> GetTris();
    std::vector<Vertex> GetAABBVertices();

    void PrintMeshNames();

    float m_hackTimer = 0;

    void UpdateRenderItems();
private:
    std::vector<RenderItem3D> m_renderItems;
    std::vector<RenderItem3D> m_renderItemsBlended;
    std::vector<RenderItem3D> m_renderItemsAlphaDiscarded;
    bool _wasPickedUpLastFrame = false;
    bool _wasRespawnedUpLastFrame = false;
    bool _wakeOnStart = false;
};