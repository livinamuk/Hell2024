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
	Transform _transform;
	Transform _openTransform;
	std::string _name = "undefined";
	std::string _parentName = "undefined";
	std::string _scriptName = "undefined";
	OpenState _openState = OpenState::NONE;
	OpenAxis _openAxis = OpenAxis::NONE;
	float _maxOpenAmount = 0;
	float _minOpenAmount = 0;
	float _openSpeed = 0;
	bool collectable = false;
	bool collected = false;


private:
	std::function<void(void)> _interactCallback = nullptr;




	//InteractType _interactType = InteractType::NONE;
	
	ModelMatrixMode _modelMatrixMode = ModelMatrixMode::GAME_TRANSFORM;
	BoundingBox _boundingBox;

	PxRigidBody* _collisionBody = NULL;
	std::vector<PxShape*> _collisionShapes;
	PxRigidStatic* _raycastBody = NULL;
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
	//void SetBoundingBoxFromMesh(int meshIndex);
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

	void CreateRigidBody(glm::mat4 matrix, bool kinematic);
	void AddCollisionShape(PxShape* shape, PhysicsFilterData physicsFilterData);
	void AddCollisionShapeFromConvexMesh(Mesh* mesh, PhysicsFilterData physicsFilterData);
	void AddCollisionShapeFromBoundingBox(BoundingBox& boundignBox, PhysicsFilterData physicsFilterData);
	void UpdateRigidBodyMassAndInertia(float density);

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