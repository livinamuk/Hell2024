#pragma once
#include "GameObject.h"
#include "../Util.hpp"
#include "Scene.h"
//#include "Callbacks.hpp"

GameObject::GameObject() {
}

void GameObject::SetModelMatrixTransformOverride(glm::mat4 model) {
	//_overrideTransformWithMatrix = true;
	//_modelMatrixTransformOverride = model;
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

void GameObject::SetScriptName(std::string name) {
	_scriptName = name;
}
bool GameObject::IsInteractable() {
	if (_openState == OpenState::CLOSED ||
		_openState == OpenState::OPEN ||
		_openState == OpenState::CLOSING ||
		_openState == OpenState::OPENING ||
		_interactType == InteractType::PICKUP ||
		_interactType == InteractType::TEXT ||
		_interactType == InteractType::QUESTION ||
		_interactType == InteractType::CALLBACK_ONLY)
		return true;
	return false;
}

void GameObject::Interact() {
	// Open
	if (_openState == OpenState::CLOSED) {
		_openState = OpenState::OPENING;
		Audio::PlayAudio(_audio.onOpen.filename, _audio.onOpen.volume);
	}
	// Close
	else if (_openState == OpenState::OPEN) {
		_openState = OpenState::CLOSING;
		Audio::PlayAudio(_audio.onClose.filename, _audio.onClose.volume);
	}
}

void GameObject::Update(float deltaTime) {
	// Open/Close if applicable
	if (_openState != OpenState::NONE) {
		// Rotation
		if (_openAxis == OpenAxis::ROTATION_NEG_Y) {
			if (_openState == OpenState::OPENING) {
				_openTransform.rotation.y -= _openSpeed * deltaTime;
			}
			if (_openState == OpenState::CLOSING) {
				_openTransform.rotation.y += _openSpeed * deltaTime;
			}
			if (_openTransform.rotation.y < _maxOpenAmount) {
				_openTransform.rotation.y = _maxOpenAmount;
				_openState = OpenState::OPEN;
			}
			if (_openTransform.rotation.y > _minOpenAmount) {
				_openTransform.rotation.y = _minOpenAmount;
				_openState = OpenState::CLOSED;
			}
		}
		/*if (_openAxis == OpenAxis::ROTATION_POS_Y) {
			if (_openState == OpenState::OPENING) {
				_transform.rotation.y += _openSpeed * deltaTime;
			}
			if (_openState == OpenState::CLOSING) {
				_transform.rotation.y -= _openSpeed * deltaTime;
			}
			if (_transform.rotation.y > _maxOpenAmount) {
				_transform.rotation.y = _maxOpenAmount;
				_openState = OpenState::OPEN;
			}
			if (_transform.rotation.y < _minOpenAmount) {
				_transform.rotation.y = _minOpenAmount;
				_openState = OpenState::CLOSED;
			}
		}
		if (_openAxis == OpenAxis::ROTATION_NEG_X) {
			if (_openState == OpenState::OPENING) {
				_transform.rotation.x -= _openSpeed * deltaTime;
			}
			if (_openState == OpenState::CLOSING) {
				_transform.rotation.x += _openSpeed * deltaTime;
			}
			if (_transform.rotation.x < _maxOpenAmount) {
				_transform.rotation.x = _maxOpenAmount;
				_openState = OpenState::OPEN;
				//GameData::_toiletLidUp = false;
			}
			if (_transform.rotation.x > _minOpenAmount) {
				_transform.rotation.x = _minOpenAmount;
				_openState = OpenState::CLOSED;
				//GameData::_toiletLidUp = true;
			}
		}
		if (_openAxis == OpenAxis::ROTATION_POS_X) {
			if (_openState == OpenState::OPENING) {
				_transform.rotation.x += _openSpeed * deltaTime;
			}
			if (_openState == OpenState::CLOSING) {
				_transform.rotation.x -= _openSpeed * deltaTime;
			}
			if (_transform.rotation.x > _maxOpenAmount) {
				_transform.rotation.x = _maxOpenAmount;
				_openState = OpenState::OPEN;
			}
			if (_transform.rotation.x < _minOpenAmount) {
				_transform.rotation.x = _minOpenAmount;
				_openState = OpenState::CLOSED;
			}
		}
		// Position
		else if (_openAxis == OpenAxis::TRANSLATE_Z) {
			if (_openState == OpenState::OPENING) {
				_transform.position.z += _openSpeed * deltaTime;
			}
			if (_openState == OpenState::CLOSING) {
				_transform.position.z -= _openSpeed * deltaTime;
			}
			if (_transform.position.z > _maxOpenAmount) {
				_transform.position.z = _maxOpenAmount;
				_openState = OpenState::OPEN;
			}
			if (_transform.position.z < 0) {
				_transform.position.z = 0;
				_openState = OpenState::CLOSED;
			}
		}*/
	}

	
	else if (_scriptName == "OpenableDrawer") {
		if (_openState == OpenState::OPENING) {
			_transform.position.z += _openSpeed * deltaTime;
		}
		if (_openState == OpenState::CLOSING) {
			_transform.position.z -= _openSpeed * deltaTime;
		}
		if (_transform.position.z > _maxOpenAmount) {
			_transform.position.z = _maxOpenAmount;
			_openState = OpenState::OPEN;
		}
		if (_transform.position.z < 0) {
			_transform.position.z = 0;
			_openState = OpenState::CLOSED;
		}
	}
	else if (_scriptName == "OpenableCabinet") {
		if (_openState == OpenState::OPENING) {
			_transform.rotation.y += _openSpeed * deltaTime;
		}
		if (_openState == OpenState::CLOSING) {
			_transform.rotation.y -= _openSpeed * deltaTime;
		}
		if (_transform.rotation.y > _maxOpenAmount) {
			_transform.rotation.y = _maxOpenAmount;
			_openState = OpenState::OPEN;
		}
		if (_transform.rotation.y < _minOpenAmount) {
			_transform.rotation.y = _minOpenAmount;
			_openState = OpenState::CLOSED;
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
		_raycastBody->userData = this;
	}
}

glm::mat4 GameObject::GetGameWorldMatrix() {
	glm::mat4 result = glm::mat4(1);
	GameObject* parent = Scene::GetGameObjectByName(_parentName);
	if (parent) {
		result = parent->GetGameWorldMatrix() * _transform.to_mat4() * _openTransform.to_mat4();
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
	_model = &AssetManager::GetModel(name);


	if (_model) {
		_meshMaterialIndices.resize(_model->_meshes.size());
		//_meshMaterialTypes.resize(_model->_meshIndices.size());
		//_meshTransforms.resize(_model->_meshIndices.size());
	}
	else {
		std::cout << "Failed to set model '" << name << "', it does not exist.\n";
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

void GameObject::PickUp() {
	//GameData::AddInventoryItem(GetName());
	//Audio::PlayAudio("ItemPickUp.wav", 0.5f);
	_collected = true;
	std::cout << "Picked up \"" << GetName() << "\"\n";
	std::cout << "_collected \"" << _collected << "\"\n";
	//std::cout << "Picked up \"" << _transform.position.x << " " <<_transform.position.y << " " << _transform.position.z << " " << "\"\n";
}

void GameObject::SetCollectedState(bool value) {
	_collected = value;
}

void GameObject::SetInteract(InteractType type, std::string text, std::function<void(void)> callback) {
	_interactType = type;
	_interactText = text;
	_interactCallback = callback;
}

void GameObject::SetBoundingBoxFromMesh(int meshIndex) {
	/*
	Mesh* mesh = AssetManager::GetMesh(_model->_meshIndices[meshIndex]);	
	std::vector<Vertex>& vertices = AssetManager::GetVertices_TEMPORARY();
	std::vector<uint32_t>& indices = AssetManager::GetIndices_TEMPORARY();

	int firstIndex = mesh->_indexOffset;
	int lastIndex = firstIndex + (int)mesh->_indexCount;

	for (int i = firstIndex; i < lastIndex; i++) {
		_boundingBox.xLow = std::min(_boundingBox.xLow, vertices[indices[i] + mesh->_vertexOffset].position.x);
		_boundingBox.xHigh = std::max(_boundingBox.xHigh, vertices[indices[i] + mesh->_vertexOffset].position.x);
		_boundingBox.zLow = std::min(_boundingBox.zLow, vertices[indices[i] + mesh->_vertexOffset].position.z);
		_boundingBox.zHigh = std::max(_boundingBox.zHigh, vertices[indices[i] + mesh->_vertexOffset].position.z);
	}	
	*/
	/*
	std::cout << "\n" << GetName() << "\n";
	std::cout << " meshIndex: " << meshIndex << "\n";
	std::cout << " firstIndex: " << firstIndex << "\n";
	std::cout << " lastIndex: " << lastIndex << "\n";
	std::cout << " _boundingBox.xLow: " << _boundingBox.xLow << "\n";
	std::cout << " _boundingBox.xHigh: " << _boundingBox.xHigh << "\n";
	std::cout << " _boundingBox.zLow: " << _boundingBox.zLow << "\n";
	std::cout << " _boundingBox.zHigh: " << _boundingBox.zHigh << "\n";*/
}

BoundingBox GameObject::GetBoundingBox() {
	return _boundingBox;
}

void GameObject::EnableCollision() {
	_collisionEnabled = true;
}

void GameObject::DisableCollision() {
	_collisionEnabled = false;
}

bool GameObject::HasCollisionsEnabled() {
	return _collisionEnabled;
}

bool GameObject::IsCollected() {
	return _collected;
}

const InteractType& GameObject::GetInteractType() {
	return _interactType;
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


void GameObject::SetMeshMaterialByMeshName(std::string meshName, std::string materialName) {
	/*	if (_model && AssetManager::GetMaterial(materialName)) {
		for(int i = 0; i < _model->_meshNames.size(); i++) {
			if (_model->_meshNames[i] == meshName) {
				_meshMaterialIndices[i] = AssetManager::GetMaterialIndex(materialName);
			}
		}
	}*/
}

void GameObject::SetCollisionShape(Transform transform, PxShape* shape, PhysicsFilterData physicsFilterData, bool kinematic) {

	if (_collisionBody) {
		_collisionBody->release();
	}
	_collisionShape = shape;
	_collisionBody = Physics::CreateRigidDynamic(transform, physicsFilterData, shape);
	_collisionBody->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, kinematic);
}

void GameObject::SetCollisionShapeFromConvexHull(Transform transform, Mesh* mesh, PhysicsFilterData physicsFilterData, bool kinematic) {

	/*if (!mesh) {
		return;
	}

	if (_collisionBody) {
		_collisionBody->release();
	}
	Mesh& mesh = _model->_meshes[meshIndex];
	_collisionShape = Physics::CreateConvexFromTriangleMesh(mesh._triangleMesh);
	_collisionBody = Physics::CreateRigidDynamic(transform, physicsFilterData, _collisionShape);
	_collisionBody->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, kinematic);

	return;*/

	/*
	if (!_model || meshIndex < 0 || meshIndex >= _model->_meshes.size()) {
		return;
	}

	Mesh& mesh = _model->_meshes[meshIndex];
	if (!mesh._triangleMesh) {
		mesh.CreateTriangleMesh();
	}
	if (_collisionBody) {
		_collisionBody->release();
	}
	if (mesh._triangleMesh) {
		_collisionShape = Physics::CreateShapeFromTriangleMesh(mesh._triangleMesh);
	}
	_collisionBody = Physics::CreateRigidDynamic(transform, physicsFilterData, _collisionShape);
	_collisionBody->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, kinematic);*/
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
	_raycastShape = Physics::CreateShapeFromTriangleMesh(mesh->_triangleMesh);
	_raycastBody = Physics::CreateRigidDynamic(Transform(), filterData, _raycastShape);
	_raycastBody->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
}

void GameObject::SetRaycastShapeFromModel(Model* model) {
	if (!model) {
		return;
	}
	if (_raycastBody) {
		_raycastBody->release();
	}
	if (!model->_triangleMesh) {
		model->CreateTriangleMesh();
	}
	PhysicsFilterData filterData;
	filterData.raycastGroup = RAYCAST_ENABLED;
	filterData.collisionGroup = CollisionGroup::NO_COLLISION;
	filterData.collidesWith = CollisionGroup::NO_COLLISION;
	_raycastShape = Physics::CreateShapeFromTriangleMesh(model->_triangleMesh);
	_raycastBody = Physics::CreateRigidDynamic(Transform(), filterData, _raycastShape);
	_raycastBody->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
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
	_raycastBody = Physics::CreateRigidDynamic(Transform(), filterData, shape);
	_raycastBody->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
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
	if (_collisionShape) {
		_collisionShape->release();
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