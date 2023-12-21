#pragma once
#include "AssetManager.h"
#include "Audio.hpp"
#include "Physics.h"

enum class OpenState { NONE, CLOSED, CLOSING, OPEN, OPENING };
enum class OpenAxis { NONE, TRANSLATE_X, TRANSLATE_Y, TRANSLATE_Z, ROTATION_POS_X, ROTATION_POS_Y, ROTATION_POS_Z, ROTATION_NEG_X, ROTATION_NEG_Y, ROTATION_NEG_Z };
enum class InteractType { NONE, TEXT, QUESTION, PICKUP, CALLBACK_ONLY };
enum class ModelMatrixMode { GAME_TRANSFORM, PHYSX_TRANSFORM };

/*struct BoundingBox {
	float xLow = 0;
	float xHigh = 0;
	float zLow = 0;
	float zHigh = 0;
};*/

struct GameObject {
public:
	Model* _model = nullptr;
	std::vector<int> _meshMaterialIndices;
	//std::vector<MaterialType> _meshMaterialTypes;
	//std::vector<Transform> _meshTransforms;
	//bool _overrideTransformWithMatrix = false;
	//std::string _interactAffectsThisObjectInstead = "";

	Transform _transform;
	Transform _openTransform;

private:
	std::function<void(void)> _interactCallback = nullptr;
	//callback_function _pickupCallback = nullptr;
	std::string _name = "undefined";
	std::string _parentName = "undefined";
	std::string _scriptName = "undefined";
	std::string _interactText = "";
	//std::string _interactTextOLD = "";
	//std::string _questionText = "";
	glm::mat4 _modelMatrixTransformOverride = glm::mat4(1);
	OpenState _openState = OpenState::NONE;
	OpenAxis _openAxis = OpenAxis::NONE;
	float _maxOpenAmount = 0;
	float _minOpenAmount = 0;
	float _openSpeed = 0;
	bool _collected = false; // if this were an item
	BoundingBox _boundingBox;
	bool _collisionEnabled = true;
	InteractType _interactType = InteractType::NONE;
	//MaterialType _materialType = MaterialType::DEFAULT;
	//PxRigidDynamic* _rigidBody = nullptr;
	ModelMatrixMode _modelMatrixMode = ModelMatrixMode::GAME_TRANSFORM;

	PxRigidBody* _collisionBody = NULL;
	PxShape* _collisionShape = NULL;
	PxRigidBody* _raycastBody = NULL;
	PxShape* _raycastShape = NULL;

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
	void SetModelMatrixTransformOverride(glm::mat4 model);
	void SetOpenAxis(OpenAxis openAxis);
	void SetAudioOnInteract(std::string filename, float volume);
	void SetAudioOnOpen(std::string filename, float volume);
	void SetAudioOnClose(std::string filename, float volume);
	void SetInteract(InteractType type, std::string text, std::function<void(void)> callback);
	//void SetPickUpQuestion(std::string text);
	//void SetPickUpCallback(callback_function callback);
	void SetOpenState(OpenState openState, float speed, float min, float max);
	void SetPosition(glm::vec3 position);
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
	//glm::vec3 GetPosition();
	//glm::mat4 GetRotationMatrix();
	void SetScale(glm::vec3 scale);
	void SetScale(float scale);
	void SetScaleX(float scale);
	//void SetInteractText(std::string text);
	//void SetPickUpText(std::string text);
	void SetName(std::string name);
	void SetParentName(std::string name);
	std::string GetParentName();
	void SetScriptName(std::string name);
	bool IsInteractable();
	void Interact();
	void Update(float deltaTime);
	void SetModel(const std::string& name);
	void SetBoundingBoxFromMesh(int meshIndex);
	//VkTransformMatrixKHR GetVkTransformMatrixKHR();
	void SetMeshMaterial(const char* name, int meshIndex = -1);
	//Material* GetMaterial(int meshIndex);
	void PickUp();
	void SetCollectedState(bool value);
	BoundingBox GetBoundingBox();
	void EnableCollision();
	void DisableCollision();
	bool HasCollisionsEnabled();
	bool IsCollected();
	const InteractType& GetInteractType();
	OpenState& GetOpenState();
	//void SetMaterialType(MaterialType materialType, int meshIndex = -1);
	void SetTransform(Transform& transform);
	void SetInteractToAffectAnotherObject(std::string objectName);
	void SetMeshMaterialByMeshName(std::string meshName, std::string materialName);
	void SetCollisionShape(Transform transform, PxShape* shape, PhysicsFilterData physicsFilterData, bool kinematic);
	void SetCollisionShapeFromConvexHull(Transform transform, Mesh* mesh, PhysicsFilterData physicsFilterData, bool kinematic);

	void SetRaycastShapeFromMesh(Mesh* mesh);
	void SetRaycastShapeFromModel(Model* model);
	void SetRaycastShape(PxShape* shape);
	
	void SetModelMatrixMode(ModelMatrixMode modelMatrixMode);
	void SetPhysicsTransform(glm::mat4 worldMatrix);
	glm::mat4 GetGameWorldMatrix(); // aka not the physx matrix
	void AddForceToCollisionObject(glm::vec3 direction, float strength);

	void CleanUp();

	std::vector<Triangle> GetTris();

private: 
	//glm::mat4 CalculateModelMatrix(ModelMatrixMode modelMatrixMode);
};