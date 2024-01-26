#pragma once
#include "GameObject.h"
#include "../Util.hpp"
#include "Scene.h"
#include "Config.hpp"
#include "Input.h"
//#include "Callbacks.hpp"

GameObject::GameObject() {
}

void GameObject::SetOpenAxis(OpenAxis openAxis) {
	_openAxis = openAxis;
}

void GameObject::SetPosition(float x, float y, float z) {
	_transform.position = glm::vec3(x, y, z);
}

void GameObject::SetPosition(glm::vec3 position) {
	_transform.position = position;
}

void GameObject::SetPositionX(float position) {
	_transform.position.x = position;
}

void GameObject::SetPositionY(float position) {
	_transform.position.y = position;
}

void GameObject::SetPositionZ(float position) {
	_transform.position.z = position;
}

void GameObject::SetRotationX(float rotation) {
	_transform.rotation.x = rotation;
}

void GameObject::SetRotationY(float rotation) {
	_transform.rotation.y = rotation;
}

void GameObject::SetRotationZ(float rotation) {
	_transform.rotation.z = rotation;
}

float GameObject::GetRotationX() {
	return _transform.rotation.x;
}

float GameObject::GetRotationY() {
	return _transform.rotation.y;
}

float GameObject::GetRotationZ() {
	return _transform.rotation.z;
}

// these below are broken

//glm::vec3 GameObject::GetPosition() {
//	return _transform.position;
//}


/*glm::mat4 GameObject::GetRotationMatrix() {
	Transform transform;
	transform.rotation = _transform.rotation;
	return transform.to_mat4();
}*/

void GameObject::SetScale(glm::vec3 scale) {
	_transform.scale = glm::vec3(scale);
}

void GameObject::SetScale(float scale) {
	_transform.scale = glm::vec3(scale);
}
void GameObject::SetScaleX(float scale) {
	_transform.scale.x = scale;
}

glm::mat4 GameObject::GetModelMatrix() {
	return _modelMatrix;
}

std::string GameObject::GetName() {
	return _name;
}

void GameObject::SetName(std::string name) {
	_name = name;
}

/*void GameObject::SetInteractText(std::string text) {
	_interactTextOLD = text;
}*/

void GameObject::SetParentName(std::string name) {
	_parentName = name;
}

std::string GameObject::GetParentName() {
	return _parentName;
}

bool GameObject::IsInteractable() {
	if (_openState == OpenState::CLOSED ||
		_openState == OpenState::OPEN ||
 		_openState == OpenState::CLOSING ||
		_openState == OpenState::OPENING)
		//_interactType == InteractType::PICKUP ||
		//_interactType == InteractType::TEXT ||
		//_interactType == InteractType::QUESTION ||
		//_interactType == InteractType::CALLBACK_ONLY)
		return true;
	return false;
}

void GameObject::Interact() {
	// Open
	if (_openState == OpenState::CLOSED) {
		_openState = OpenState::OPENING;
		Audio::PlayAudio("DrawerOpen.wav", 1.0f);
	}
	// Close
	else if (_openState == OpenState::OPEN) {
		Audio::PlayAudio("DrawerClose.wav", 1.0f); 
		_openState = OpenState::CLOSING;
		//Audio::PlayAudio(_audio.onClose.filename, _audio.onClose.volume);
	}
}

void GameObject::Update(float deltaTime) {
	
	if (_pickupType != PickUpType::NONE) {
		if (_pickupCoolDownTime > 0) {
			_pickupCoolDownTime -= deltaTime;
		}
		if (_pickupCoolDownTime <= 0) {
			_pickupCoolDownTime = 0;
			_collected = false;
		}
	}

	// Open/Close if applicable
	if (_openState != OpenState::NONE) {

		if (_openState == OpenState::OPENING) {

			float speed = 3.0f;
			float maxOpenDistance = 0.25f;

			if (_openTransform.position.z < maxOpenDistance) {
				_openTransform.position.z += deltaTime * speed;
			}
			
			if (_openTransform.position.z >= maxOpenDistance) {
				_openState = OpenState::OPEN;
				_openTransform.position.z = std::min(_openTransform.position.z, maxOpenDistance);
			}
		}

		if (_openState == OpenState::CLOSING) {

			float speed = 3.0f;

			if (_openTransform.position.z > 0) {
				_openTransform.position.z -= deltaTime * speed;
			}

			if (_openTransform.position.z <= 0) {
				_openState = OpenState::CLOSED;
				_openTransform.position.z = 0;
			}
		}
	}

	if (_modelMatrixMode == ModelMatrixMode::GAME_TRANSFORM) {
		_modelMatrix = GetGameWorldMatrix();
		if (_collisionBody) {
			_collisionBody->setGlobalPose(PxTransform(Util::GlmMat4ToPxMat44(_modelMatrix)));
		}
		if (_raycastBody) {
			_raycastBody->setGlobalPose(PxTransform(Util::GlmMat4ToPxMat44(_modelMatrix)));
		}
	}
	else if (_modelMatrixMode == ModelMatrixMode::PHYSX_TRANSFORM && _collisionBody) {
		Transform transform;
		transform.scale = _transform.scale;
		_modelMatrix = Util::PxMat44ToGlmMat4(_collisionBody->getGlobalPose()) * transform.to_mat4();

		if (_raycastBody) {
			_raycastBody->setGlobalPose(_collisionBody->getGlobalPose());
		}
	}
	// Pointers
	if (_raycastBody) {
		PhysicsObjectData* physicsObjectData = (PhysicsObjectData*)_raycastBody->userData;
		physicsObjectData->type = GAME_OBJECT;
		physicsObjectData->parent = this;
	}
}

void GameObject::UpdateEditorPhysicsObject() {
	if (_editorRaycastBody) {
		if (_modelMatrixMode == ModelMatrixMode::GAME_TRANSFORM) {
			PxQuat quat = Util::GlmQuatToPxQuat(GetWorldRotation());
			PxVec3 position = Util::GlmVec3toPxVec3(GetWorldPosition());
			PxTransform transform = PxTransform(position, quat);
			_editorRaycastBody->setGlobalPose(transform);
		}
		else if (_modelMatrixMode == ModelMatrixMode::PHYSX_TRANSFORM && _collisionBody) {
			_editorRaycastBody->setGlobalPose(_collisionBody->getGlobalPose());
		}
	}
}

glm::vec3 GameObject::GetWorldPosition() {
	return Util::GetTranslationFromMatrix(GetGameWorldMatrix());
}

glm::quat GameObject::GetWorldRotation() {

	glm::quat result = glm::quat(0, 0, 0, 1);
	if (_parentName != "undefined") {
		GameObject* parent = Scene::GetGameObjectByName(_parentName);
		if (parent) {
			result = parent->GetWorldRotation() * glm::quat(_transform.rotation) * glm::quat(_openTransform.rotation);
		}
	}
	else {
		result = glm::quat(_transform.rotation);
	}
	return result;
}

glm::mat4 GameObject::GetGameWorldMatrix() {
	glm::mat4 result = glm::mat4(1);
	if (_parentName != "undefined") {
		GameObject* parent = Scene::GetGameObjectByName(_parentName);
		if (parent) {
			result = parent->GetGameWorldMatrix() * _transform.to_mat4() * _openTransform.to_mat4();
		}
	}
	else {
		result = _transform.to_mat4();
	}
	return result;
}

void GameObject::AddForceToCollisionObject(glm::vec3 direction, float strength) {
	if (!_collisionBody) {
		return;
	}
	auto flags = _collisionBody->getRigidBodyFlags();
	if (flags & PxRigidBodyFlag::eKINEMATIC) {
		return;
	}
	PxVec3 force = PxVec3(direction.x, direction.y, direction.z) * strength; 
	_collisionBody->addForce(force);
}

void GameObject::UpdateRigidBodyMassAndInertia(float density) {
	if (_collisionBody) {
		PxRigidBodyExt::updateMassAndInertia(*_collisionBody, density);
	}
}


void GameObject::SetAudioOnOpen(std::string filename, float volume) {
	_audio.onOpen = { filename, volume };
}

void GameObject::SetAudioOnClose(std::string filename, float volume) {
	_audio.onClose = { filename, volume };
}

void GameObject::SetAudioOnInteract(std::string filename, float volume) {
	_audio.onInteract = { filename, volume };
}


void GameObject::SetOpenState(OpenState openState, float speed, float min, float max) {
	_openState = openState;
	_openSpeed = speed;
	_minOpenAmount = min;
	_maxOpenAmount = max;
}

void GameObject::SetModel(const std::string& name)
{
	_model = AssetManager::GetModel(name);

	if (_model) {
		_meshMaterialIndices.resize(_model->_meshes.size());
		//_meshMaterialTypes.resize(_model->_meshIndices.size());
		//_meshTransforms.resize(_model->_meshIndices.size());
	}
	else {
		std::cout << "Failed to set model '" << name << "', it does not exist.\n";
	}
}

void GameObject::SetMeshMaterialByMeshName(std::string meshName, std::string materialName) {
	int materialIndex = AssetManager::GetMaterialIndex(materialName);
	if (_model && materialIndex != -1) {
		for (int i = 0; i < _model->_meshes.size(); i++) {
			if (_model->_meshes[i]._name == meshName) {
				_meshMaterialIndices[i] = materialIndex;
				return;
			}
		}
	}
	if (!_model) {
		std::cout << "Tried to call SetMeshMaterialByMeshName() but this GameObject has a nullptr model\n";
	}
	if (materialIndex == -1) {
		std::cout << "Tried to call SetMeshMaterialByMeshName() but the material index was -1\n";
	}
}


void GameObject::SetMeshMaterial(const char* name, int meshIndex) {
	// Range checks
	if (!_meshMaterialIndices.size()) {
		std::cout << "Failed to set mesh material, GameObject has _meshMaterials with size " << _meshMaterialIndices.size() << "\n";
		return;
	}
	else if (meshIndex != -1 && meshIndex > _meshMaterialIndices.size()) {
		std::cout << "Failed to set mesh material with mesh Index " << meshIndex << ", GameObject has _meshMaterials with size " << _meshMaterialIndices.size() << "\n";
		return;
	}
	// Get the material index via the material name
	int materialIndex = AssetManager::GetMaterialIndex(name);
	// Set material for single mesh
	if (meshIndex != -1) {
		_meshMaterialIndices[meshIndex] = materialIndex;
	}
	// Set material for all mesh
	if (meshIndex == -1) {
		for (int i = 0; i < _meshMaterialIndices.size(); i++) {
			_meshMaterialIndices[i] = materialIndex;
		}
	}
}
/*
Material* GameObject::GetMaterial(int meshIndex) {
	if (meshIndex < 0 || meshIndex >= _meshMaterialIndices.size()) {
		std::cout << "Mesh index " << meshIndex << " is out of range of _meshMaterialIndices with size " << _meshMaterialIndices.size() << "\n";
	}
	int materialIndex = _meshMaterialIndices[meshIndex];
	return AssetManager::GetMaterial(materialIndex);
}*/

//#include "GameData.h"


void GameObject::SetCollectedState(bool value) {
	//_collected = value;
}


BoundingBox GameObject::GetBoundingBox() {
	return _boundingBox;
}

bool GameObject::IsCollected() {
	return _collected;
}

PickUpType GameObject::GetPickUpType() {
	return _pickupType;
}

void GameObject::CreateEditorPhysicsObject() {

	if (_editorRaycastBody) {
		_editorRaycastBody->release();
	}
	if (_editorRaycastShape) {
		_editorRaycastShape->release();
	}
	if (!_model->_triangleMesh) {
		_model->CreateTriangleMesh();
	}
	PxShapeFlags shapeFlags(PxShapeFlag::eSCENE_QUERY_SHAPE);
	_editorRaycastShape = Physics::CreateShapeFromTriangleMesh(_model->_triangleMesh, shapeFlags, Physics::GetDefaultMaterial(), _transform.scale);
	_editorRaycastBody = Physics::CreateEditorRigidStatic(_transform, _editorRaycastShape);
	if (_editorRaycastBody->userData) {
		delete _editorRaycastBody->userData;
	}
	_editorRaycastBody->userData = new PhysicsObjectData(PhysicsObjectType::GAME_OBJECT, this);

	//std::cout << "created editor object for GameObject with model " << _model->_name << "\n";
}

const InteractType& GameObject::GetInteractType() {
	return InteractType::NONE;
	//return _interactType;
}

OpenState& GameObject::GetOpenState() {
	return _openState;
}

/*
void GameObject::SetMaterialType(MaterialType materialType, int meshIndex)
{
	if (meshIndex == -1) {
		for (int i = 0; i < _meshMaterialTypes.size(); i++) {
			_meshMaterialTypes[i] = materialType;
		}
	}
	else if (meshIndex < _meshMaterialTypes.size()) {
		_meshMaterialTypes[meshIndex] = materialType;
	}
	else {
		std::cout << "Error in SetMaterialType(), meshIndex " << meshIndex << "is out of range of. _meshMaterialTypes size is " << _meshMaterialTypes.size() << "\n";
	}
}*/

void GameObject::SetTransform(Transform& transform)
{
	_transform = transform;
}

void GameObject::SetInteractToAffectAnotherObject(std::string objectName)
{
	//_interactAffectsThisObjectInstead = objectName;
}



void GameObject::AddCollisionShape(PxShape* shape, PhysicsFilterData physicsFilterData) {
	if (!_collisionBody) {
		std::cout << "You tried to add a collision shape to a GameObject without a rigid body. GameObject name is '" << _name << "'\n";
		return;
	}
	PxFilterData filterData;
	filterData.word0 = (PxU32)physicsFilterData.raycastGroup;
	filterData.word1 = (PxU32)physicsFilterData.collisionGroup;
	filterData.word2 = (PxU32)physicsFilterData.collidesWith;
	shape->setQueryFilterData(filterData);       // ray casts
	shape->setSimulationFilterData(filterData);  // collisions
	shape = shape;
	_collisionShapes.push_back(shape);
	_collisionBody->attachShape(*shape);
}

void GameObject::CreateRigidBody(glm::mat4 matrix, bool kinematic) {
	if (_collisionBody) {
		_collisionBody->release();
	}
	_collisionBody = Physics::CreateRigidDynamic(matrix, kinematic);
}

void GameObject::AddCollisionShapeFromConvexMesh(Mesh* mesh, PhysicsFilterData physicsFilterData) {
	if (!_collisionBody) {
		std::cout << "You tried to add a ConvexMesh shape to a GameObject without a rigid body. GameObject name is '" << _name << "'\n";
		return;
	}
	if (!mesh) {
		return;
	}
	if (!mesh->_convexMesh) {
		mesh->CreateConvexMesh();
	}
	PxShape* shape = Physics::CreateShapeFromConvexMesh(mesh->_convexMesh);
	PxFilterData filterData;
	filterData.word0 = (PxU32)physicsFilterData.raycastGroup;
	filterData.word1 = (PxU32)physicsFilterData.collisionGroup;
	filterData.word2 = (PxU32)physicsFilterData.collidesWith;
	shape->setQueryFilterData(filterData);       // ray casts
	shape->setSimulationFilterData(filterData);  // collisions
	_collisionShapes.push_back(shape);
	_collisionBody->attachShape(*shape);
}

void GameObject::AddCollisionShapeFromBoundingBox(BoundingBox& boundingBox, PhysicsFilterData physicsFilterData) {
	if (!_collisionBody) {
		std::cout << "You tried to add a bounding box collision shape to a GameObject without a rigid body. GameObject name is '" << _name << "'\n";
		return;
	}
	PxShape* shape = Physics::CreateBoxShape(boundingBox.size.x * 0.5f, boundingBox.size.y * 0.5f, boundingBox.size.z * 0.5f);
	PxFilterData filterData;
	filterData.word0 = (PxU32)physicsFilterData.raycastGroup;
	filterData.word1 = (PxU32)physicsFilterData.collisionGroup;
	filterData.word2 = (PxU32)physicsFilterData.collidesWith;
	shape->setQueryFilterData(filterData);       // ray casts
	shape->setSimulationFilterData(filterData);  // collisions
	_collisionShapes.push_back(shape);
	_collisionBody->attachShape(*shape);

	Transform shapeOffset;
	shapeOffset.position = boundingBox.offsetFromModelOrigin + (boundingBox.size * glm::vec3(0.5f));
	PxMat44 localShapeMatrix = Util::GlmMat4ToPxMat44(shapeOffset.to_mat4());
	PxTransform localShapeTransform(localShapeMatrix);
	shape->setLocalPose(localShapeTransform);
}

void GameObject::SetRaycastShapeFromMesh(Mesh* mesh) {
	if (!mesh) {
		return;
	}
	if (_raycastBody) {
		_raycastBody->release();
	}
	if (!mesh->_triangleMesh) {
		mesh->CreateTriangleMesh();
	}
	PhysicsFilterData filterData;
	filterData.raycastGroup = RAYCAST_ENABLED;
	filterData.collisionGroup = CollisionGroup::NO_COLLISION;
	filterData.collidesWith = CollisionGroup::NO_COLLISION;
	PxShapeFlags shapeFlags(PxShapeFlag::eSCENE_QUERY_SHAPE); // Most importantly NOT eSIMULATION_SHAPE. PhysX does not allow for tri mesh.
	_raycastShape = Physics::CreateShapeFromTriangleMesh(mesh->_triangleMesh, shapeFlags);
	_raycastBody = Physics::CreateRigidStatic(Transform(), filterData, _raycastShape);
	_raycastBody->userData = new PhysicsObjectData(PhysicsObjectType::GAME_OBJECT, this);
}

void GameObject::SetRaycastShapeFromModel(Model* model) {
	if (!model) {
		return;
	}
	if (_raycastBody) {
		_raycastBody->release();
	}
	if (!model->_triangleMesh) {
		//std::cout << "SetRaycastShapeFromModel() for game object '" << this->_name << "' with model '" << model->_name << "'\n";
		model->CreateTriangleMesh();
	}
	PhysicsFilterData filterData;
	filterData.raycastGroup = RAYCAST_ENABLED;
	filterData.collisionGroup = CollisionGroup::NO_COLLISION;
	filterData.collidesWith = CollisionGroup::NO_COLLISION;
	PxShapeFlags shapeFlags(PxShapeFlag::eSCENE_QUERY_SHAPE); // Most importantly NOT eSIMULATION_SHAPE. PhysX does not allow for tri mesh.
	_raycastShape = Physics::CreateShapeFromTriangleMesh(model->_triangleMesh, shapeFlags);
	_raycastBody = Physics::CreateRigidStatic(Transform(), filterData, _raycastShape);
	_raycastBody->userData = new PhysicsObjectData(PhysicsObjectType::GAME_OBJECT, this);
}

void GameObject::SetRaycastShape(PxShape* shape) {
	if (!shape) {
		return;
	}
	if (_raycastBody) {
		_raycastBody->release();
	}
	PhysicsFilterData filterData;
	filterData.raycastGroup = RAYCAST_ENABLED;
	filterData.collisionGroup = CollisionGroup::NO_COLLISION;
	filterData.collidesWith = CollisionGroup::NO_COLLISION;
	_raycastBody = Physics::CreateRigidStatic(Transform(), filterData, shape);
	_raycastBody->userData = new PhysicsObjectData(PhysicsObjectType::GAME_OBJECT, this);

}

void GameObject::SetModelMatrixMode(ModelMatrixMode modelMatrixMode) {
	_modelMatrixMode = modelMatrixMode;
}

void GameObject::SetPhysicsTransform(glm::mat4 worldMatrix) {

	std::cout << "cunt\n" << Util::Mat4ToString(worldMatrix) << "\n";

	_collisionBody->setGlobalPose(PxTransform(Util::GlmMat4ToPxMat44(worldMatrix)));
}


void GameObject::CleanUp() {
	if (_collisionBody) {
		_collisionBody->release();
	}
	for (auto* shape : _collisionShapes) {
		if (shape) {
			if (shape) {
				shape->release();
			}
		}
	}
	if (_raycastBody) {
		_raycastBody->release();
	}
	if (_raycastShape) {
		_raycastShape->release();
	}
}

std::vector<Triangle> GameObject::GetTris() {

	const float doorWidth = -0.794f;
	const float doorDepth = -0.0379f;
	const float doorHeight = 2.0f;

	// Front
	glm::vec3 pos0 = glm::vec3(0, 0, 0);
	glm::vec3 pos1 = glm::vec3(0.0, 0, doorWidth);
	// Back
	glm::vec3 pos2 = glm::vec3(doorDepth, 0, 0);
	glm::vec3 pos3 = glm::vec3(doorDepth, 0, doorWidth);

	/*
	pos0 = (GetModelMatrix() * glm::vec4(pos0, 1.0));
	pos1 = (GetModelMatrix() * glm::vec4(pos1, 1.0));
	pos2 = (GetModelMatrix() * glm::vec4(pos2, 1.0));
	pos3 = (GetModelMatrix() * glm::vec4(pos3, 1.0));*/
	
	// FRONT
	Triangle triA;
	triA.p1 = pos0;
	triA.p2 = pos1;
	triA.p3 = pos1 + glm::vec3(0, doorHeight, 0);
	triA.color = YELLOW;
	Triangle triB;
	triB.p1 = pos1 + glm::vec3(0, doorHeight, 0);
	triB.p2 = pos0 + glm::vec3(0, doorHeight, 0);
	triB.p3 = pos0;
	triB.color = YELLOW;

	// BACK
	Triangle triC;
	triC.p3 = pos2;
	triC.p2 = pos3;
	triC.p1 = pos3 + glm::vec3(0, doorHeight, 0);
	triC.color = YELLOW;
	Triangle triD;
	triD.p3 = pos3 + glm::vec3(0, doorHeight, 0);
	triD.p2 = pos2 + glm::vec3(0, doorHeight, 0);
	triD.p1 = pos2;
	triD.color = YELLOW;

	std::vector<Triangle> result;
	result.push_back(triA);
	result.push_back(triB);
	result.push_back(triC);
	result.push_back(triD);

	return result;
}

void GameObject::PickUp() {
	_collected = true;
	_pickupCoolDownTime = Config::item_respawn_time;
}

void GameObject::SetPickUpType(PickUpType pickupType) {
	_pickupType = pickupType;
}

bool GameObject::IsCollectable() {
	return (_pickupType != PickUpType::NONE);
}