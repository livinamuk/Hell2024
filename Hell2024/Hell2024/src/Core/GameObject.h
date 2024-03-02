#pragma once
#include "AssetManager.h"
#include "Audio.hpp"
#include "Physics.h"

enum class OpenState { NONE, CLOSED, CLOSING, OPEN, OPENING };
enum class OpenAxis { NONE, TRANSLATE_X, TRANSLATE_Y, TRANSLATE_Z, ROTATION_POS_X, ROTATION_POS_Y, ROTATION_POS_Z, ROTATION_NEG_X, ROTATION_NEG_Y, ROTATION_NEG_Z };
enum class InteractType { NONE, TEXT, QUESTION, PICKUP, CALLBACK_ONLY };
enum class ModelMatrixMode { GAME_TRANSFORM, PHYSX_TRANSFORM };
enum class PickUpType { NONE, GLOCK, GLOCK_AMMO, SHOTGUN, SHOTGUN_AMMO, AKS74U, AKS74U_AMMO, AKS74U_SCOPE };

struct GameObject {
public:
	Model* _model = nullptr;
	std::vector<int> _meshMaterialIndices;
	Transform _transform;
	Transform _openTransform;
	std::string _name = "undefined";
	std::string _parentName = "undefined";
	OpenState _openState = OpenState::NONE;
	OpenAxis _openAxis = OpenAxis::NONE;
	float _maxOpenAmount = 0;
	float _minOpenAmount = 0;
	float _openSpeed = 0;


	// make this private again some how PLEEEEEEEEEEEEEEEEEEEEEASE when you have time. no rush.
	PxRigidBody* _collisionBody = NULL;

	PxRigidStatic* _editorRaycastBody = NULL;
	PxShape* _editorRaycastShape = NULL;

    PxRigidStatic* _raycastBody = NULL;
    PxShape* _raycastShape = NULL;

private:

	PickUpType _pickupType = PickUpType::NONE;
	bool _collected = false;
	float _pickupCoolDownTime = 0;
		
	ModelMatrixMode _modelMatrixMode = ModelMatrixMode::GAME_TRANSFORM;
	BoundingBox _boundingBox;
	std::vector<PxShape*> _collisionShapes;


	struct AudioEffects {
		AudioEffectInfo onOpen;
		AudioEffectInfo onClose;
		AudioEffectInfo onInteract;
	} _audio;

private:
	glm::mat4 _modelMatrix;


public:
	GameObject();
	glm::mat4 GetModelMatrix();
	std::string GetName();

	glm::vec3 GetWorldPosition();
	glm::quat GetWorldRotation();
	void UpdateEditorPhysicsObject();

	glm::vec3 GetWorldSpaceOABBCenter();
    void DisableRaycasting();
    void EnableRaycasting();

    AABB _aabb;
    AABB _aabbPreviousFrame;

	//void SetModelScaleWhenUsingPhysXTransform(glm::vec3 scale);

	glm::vec3 _scaleWhenUsingPhysXTransform = glm::vec3(1);

	void SetOpenAxis(OpenAxis openAxis);
	void SetAudioOnInteract(std::string filename, float volume);
	void SetAudioOnOpen(std::string filename, float volume);
	void SetAudioOnClose(std::string filename, float volume);
	void SetInteract(InteractType type, std::string text, std::function<void(void)> callback);
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
	void SetScriptName(std::string name);
	bool IsInteractable();
	void Interact();
	void Update(float deltaTime);
	void SetModel(const std::string& name);
	void SetMeshMaterial(const char* name, int meshIndex = -1);
	void SetCollectedState(bool value);
	BoundingBox GetBoundingBox();
	const InteractType& GetInteractType();
	OpenState& GetOpenState();
	void SetTransform(Transform& transform);
	void SetInteractToAffectAnotherObject(std::string objectName);
	void SetMeshMaterialByMeshName(std::string meshName, std::string materialName);
	void PickUp();
	void SetPickUpType(PickUpType pickupType);
	void DisablePickUp();
	bool IsCollectable();
	bool IsCollected();
	PickUpType GetPickUpType();
	void CreateEditorPhysicsObject();

	void CreateRigidBody(glm::mat4 matrix, bool kinematic);
	void AddCollisionShape(PxShape* shape, PhysicsFilterData physicsFilterData);
	void AddCollisionShapeFromConvexMesh(Mesh* mesh, PhysicsFilterData physicsFilterData, glm::vec3 scale = glm::vec3(1));
	void AddCollisionShapeFromBoundingBox(BoundingBox& boundignBox, PhysicsFilterData physicsFilterData);
	void UpdateRigidBodyMassAndInertia(float density);
    void PutRigidBodyToSleep();

	void SetRaycastShapeFromMesh(Mesh* mesh);
	void SetRaycastShapeFromModel(Model* model);
	void SetRaycastShape(PxShape* shape);
	
	void SetModelMatrixMode(ModelMatrixMode modelMatrixMode);
	void SetPhysicsTransform(glm::mat4 worldMatrix);
	glm::mat4 GetGameWorldMatrix(); // aka not the physx matrix
	void AddForceToCollisionObject(glm::vec3 direction, float strength);

    void CleanUp();
    bool HasMovedSinceLastFrame();

	std::vector<Triangle> GetTris();

private:
    bool _wasPickedUpLastFrame = false;
    bool _wasRespawnedUpLastFrame = false;
	//glm::mat4 CalculateModelMatrix(ModelMatrixMode modelMatrixMode);
};