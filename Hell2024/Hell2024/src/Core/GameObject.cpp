#pragma once
#include "GameObject.h"
#include "../Util.hpp"
#include "Scene.h"
#include "Config.hpp"
#include "Input.h"
#include "../EngineState.hpp"
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

void GameObject::SetRotation(glm::vec3 rotation) {
    _transform.rotation = rotation;
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

    // Scale physx objects
    if (_raycastObject.rigid) {
        // commented out below coz it fucked the phyx pointers of everything
        //UpdateRaycastObject();
    }
    // below needs it also
    // std::vector<PxShape*> _collisionShapes;
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

    _wasPickedUpLastFrame = false;
    _wasRespawnedUpLastFrame = false;

    if (_name == "GlockAmmo_PickUp") {
           
        GameObject* topDraw = Scene::GetGameObjectByName("TopDraw");
        if (topDraw) {

            glm::mat4 globalPose = Util::PxMat44ToGlmMat4(_collisionBody->getGlobalPose());        
            float width = 0.4f;
            float height = 0.4f;
            float depth = 0.4f;
            PxShape* overlapShape = Physics::CreateBoxShape(width, height, depth, Transform());
            const PxGeometry& overlapGeometry = overlapShape->getGeometry();
            const PxTransform shapePose(_collisionBody->getGlobalPose());
            OverlapReport overlapReport = Physics::OverlapTest(overlapGeometry, shapePose, CollisionGroup::GENERIC_BOUNCEABLE);

           // std::cout << "\nhit count: " << overlapReport.hits.size() << "\n";

            if (overlapReport.HitsFound()) {
                for (auto& hit : overlapReport.hits) {

                  /* std::cout << "hit object:  " << hit << "\n";

                    for (GameObject& object : Scene::_gameObjects) {
                        if (object._collisionBody == hit) {
                            std::cout << "-" << object.GetName() << "\n";
                        }
                    }
                    */
                    if (hit == topDraw->_collisionBody) {
                        float speed = 3.0f;
                        Transform displacement;

                        if (topDraw->_openState == OpenState::OPENING) {
                            displacement.position.z += deltaTime * speed;
                            PxMat44 physXGlobalPose = Util::GlmMat4ToPxMat44(globalPose * displacement.to_mat4());
                            PxTransform transform(physXGlobalPose);
                            _collisionBody->setGlobalPose(transform);
                        }
                        else if (topDraw->_openState == OpenState::CLOSING) {
                            displacement.position.z -= deltaTime * speed;
                            PxMat44 physXGlobalPose = Util::GlmMat4ToPxMat44(globalPose * displacement.to_mat4());
                            PxTransform transform(physXGlobalPose);
                            _collisionBody->setGlobalPose(transform);
                        }
                    }
                }                
            }

         //   std::cout << "ammo object: " << _collisionBody << "\n";
        }
    }
	


	if (_pickupType != PickUpType::NONE) {

        // Decrement pickup cooldown timer
		if (_pickupCoolDownTime > 0) {
			_pickupCoolDownTime -= deltaTime;
		}
        // Respawn item when timer is zero
        if (_pickupCoolDownTime < 0) {
			_pickupCoolDownTime = 0;
			_collected = false;
            EnableRaycasting();
            LoadSavedState();
            _wasPickedUpLastFrame = true;
		}
	}

	// Open/Close if applicable
	if (_openState != OpenState::NONE) {

		if (_openState == OpenState::OPENING && _openAxis != OpenAxis::ROTATION_POS_X && _openAxis != OpenAxis::ROTATION_NEG_X) {

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

		if (_openState == OpenState::CLOSING && _openAxis != OpenAxis::ROTATION_POS_X && _openAxis != OpenAxis::ROTATION_NEG_X) {

			float speed = 3.0f;

			if (_openTransform.position.z > 0) {
				_openTransform.position.z -= deltaTime * speed;
			}

			if (_openTransform.position.z <= 0) {
				_openState = OpenState::CLOSED;
				_openTransform.position.z = 0;
			}
		}




        // toilet hardcoded shit

        float maxOpenValue = HELL_PI * -0.5f - 0.0374f;
        float maxOpenValueSeat = HELL_PI * 0.5f + 0.0374f;
        float speed = 10.0f;
        if (_openState == OpenState::OPENING && _openAxis == OpenAxis::ROTATION_POS_X) {
            _openTransform.rotation.x -= deltaTime * speed;
            if (_openTransform.rotation.x < maxOpenValue) {
                _openState = OpenState::OPEN;
            }
            if (_openTransform.rotation.x < maxOpenValue) {
                _openTransform.rotation.x = maxOpenValue;
            }
        }

        if (_openState == OpenState::CLOSING && _openAxis == OpenAxis::ROTATION_POS_X) {
            _openTransform.rotation.x += deltaTime * speed;
            if (_openTransform.rotation.x > 0) {
                _openState = OpenState::CLOSED;
            }
            _openTransform.rotation.x = std::min(_openTransform.rotation.x, 0.0f);
        }


        if (_openState == OpenState::OPENING && _openAxis == OpenAxis::ROTATION_NEG_X) {
            _openTransform.rotation.x += deltaTime * speed;
            if (_openTransform.rotation.x > maxOpenValueSeat) {
                _openState = OpenState::OPEN;
            }
            if (_openTransform.rotation.x > maxOpenValueSeat) {
                _openTransform.rotation.x = maxOpenValueSeat;
            }
        }

        if (_openState == OpenState::CLOSING && _openAxis == OpenAxis::ROTATION_NEG_X) {
            _openTransform.rotation.x -= deltaTime * speed;
            if (_openTransform.rotation.x < 0) {
                _openState = OpenState::CLOSED;
            }
            _openTransform.rotation.x = std::max(_openTransform.rotation.x, 0.0f);
        }
	}

    // Editor or has no physics
	if (_modelMatrixMode == ModelMatrixMode::GAME_TRANSFORM || EngineState::GetEngineMode() == EngineMode::EDITOR) {
		_modelMatrix = GetGameWorldMatrix();
		if (_collisionBody) {
            PutRigidBodyToSleep();
			_collisionBody->setGlobalPose(PxTransform(Util::GlmMat4ToPxMat44(_modelMatrix)));
		}
		if (_raycastObject.rigid) {
            _raycastObject.rigid->setGlobalPose(PxTransform(Util::GlmMat4ToPxMat44(_modelMatrix)));
		}
	}
    // Has physics object
	else if (_modelMatrixMode == ModelMatrixMode::PHYSX_TRANSFORM && _collisionBody) {
		Transform transform;
		transform.scale = _transform.scale;
		_modelMatrix = Util::PxMat44ToGlmMat4(_collisionBody->getGlobalPose()) * transform.to_mat4();

		if (_raycastObject.rigid) {
            _raycastObject.rigid->setGlobalPose(_collisionBody->getGlobalPose());
		}
	}

	// Update raycast object PhysX pointer
	if (_raycastObject.rigid) {
		if (_raycastObject.rigid->userData) {
			delete _raycastObject.rigid->userData;
		}
        _raycastObject.rigid->userData = new PhysicsObjectData(PhysicsObjectType::GAME_OBJECT, this);
	}
	// Update collision object PhysX pointer
	if (_collisionBody) {
		if (_collisionBody->userData) {
			delete _collisionBody->userData;
		}
		_collisionBody->userData = new PhysicsObjectData(PhysicsObjectType::GAME_OBJECT, this);
	}

    // AABB
    if (_raycastObject.rigid) {
        _aabbPreviousFrame = _aabb;
        _aabb.extents = Util::PxVec3toGlmVec3(_raycastObject.rigid->getWorldBounds().getExtents());
        _aabb.position = Util::PxVec3toGlmVec3(_raycastObject.rigid->getWorldBounds().getCenter());
    }
}


void GameObject::SetWakeOnStart(bool value) {
    _wakeOnStart = value;
}

void GameObject::UpdateEditorPhysicsObject() {
	/*if (_editorRaycastBody) {
		if (_modelMatrixMode == ModelMatrixMode::GAME_TRANSFORM) {
			PxQuat quat = Util::GlmQuatToPxQuat(GetWorldRotation());
			PxVec3 position = Util::GlmVec3toPxVec3(GetWorldPosition());
			PxTransform transform = PxTransform(position, quat);
			_editorRaycastBody->setGlobalPose(transform);
		}
		else if (_modelMatrixMode == ModelMatrixMode::PHYSX_TRANSFORM && _collisionBody) {
			_editorRaycastBody->setGlobalPose(_collisionBody->getGlobalPose());
		}
		// Repair broken pointer 
		// (this happens when a mag pushes a new GameObject into the _gameObjects std::vector)
		if (_editorRaycastBody->userData) {
			delete _editorRaycastBody->userData;
		}
		_editorRaycastBody->userData = new PhysicsObjectData(PhysicsObjectType::GAME_OBJECT, this);
	}*/
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

glm::vec3 GameObject::GetWorldSpaceOABBCenter() {
	return GetWorldPosition() + _boundingBox.offsetFromModelOrigin;
}

void GameObject::SetAudioOnClose(std::string filename, float volume) {
	_audio.onClose = { filename, volume };
}

/*
void GameObject::SetModelScaleWhenUsingPhysXTransform(glm::vec3 scale)
{
}*/

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

/*
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
	_editorRaycastBody = Physics::CreateEditorRigidStatic(_transform, _editorRaycastShape, Physics::GetEditorScene());
	if (_editorRaycastBody->userData) {
		delete _editorRaycastBody->userData;
	}
	_editorRaycastBody->userData = new PhysicsObjectData(PhysicsObjectType::GAME_OBJECT, this);

	//std::cout << "created editor object for GameObject with model " << _model->_name << "\n";
}*/

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


void GameObject::DisableRespawnOnPickup() {
    _respawns = false;
}

void GameObject::CreateRigidBody(glm::mat4 matrix, bool kinematic) {
	if (_collisionBody) {
		_collisionBody->release();
	}
	_collisionBody = Physics::CreateRigidDynamic(matrix, kinematic);
}

void GameObject::AddCollisionShapeFromConvexMesh(Mesh* mesh, PhysicsFilterData physicsFilterData, glm::vec3 scale) {
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
	PxShape* shape = Physics::CreateShapeFromConvexMesh(mesh->_convexMesh, NULL, scale);
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
	if (_raycastObject.rigid) {
        _raycastObject.rigid->release();
	}
	if (!mesh->_triangleMesh) {
		mesh->CreateTriangleMesh();
	}
	PhysicsFilterData filterData;
	filterData.raycastGroup = RAYCAST_ENABLED;
	filterData.collisionGroup = CollisionGroup::NO_COLLISION;
	filterData.collidesWith = CollisionGroup::NO_COLLISION;
	PxShapeFlags shapeFlags(PxShapeFlag::eSCENE_QUERY_SHAPE); // Most importantly NOT eSIMULATION_SHAPE. PhysX does not allow for tri mesh.
    _raycastObject.shape = Physics::CreateShapeFromTriangleMesh(mesh->_triangleMesh, shapeFlags);
    _raycastObject.rigid = Physics::CreateRigidStatic(Transform(), filterData, _raycastObject.shape);
    _raycastObject.rigid->userData = new PhysicsObjectData(PhysicsObjectType::GAME_OBJECT, this);
}

void GameObject::UpdateRaycastObject() {

    // REMOVE THIS! ITS A HACK CAUSE YOU HAVEN'T GOT THIS WORKING WHEN THE SHAPE IS NOT A TRI MESH
    if (_model && _model->_name == "SmallCube") {
        return;
    }

    if (_raycastObject.rigid) {
        _raycastObject.rigid->release();
    }
    if (!_raycastObject.model->_triangleMesh) {
        _raycastObject.model->CreateTriangleMesh();
    }
    
    PhysicsFilterData filterData;
    filterData.raycastGroup = RAYCAST_ENABLED;
    filterData.collisionGroup = CollisionGroup::NO_COLLISION;
    filterData.collidesWith = CollisionGroup::NO_COLLISION;
    PxShapeFlags shapeFlags(PxShapeFlag::eSCENE_QUERY_SHAPE); // Most importantly NOT eSIMULATION_SHAPE. PhysX does not allow for tri mesh.
    _raycastObject.shape = Physics::CreateShapeFromTriangleMesh(_raycastObject.model->_triangleMesh, shapeFlags, Physics::GetDefaultMaterial(), _transform.scale);
    _raycastObject.rigid = Physics::CreateRigidStatic(Transform(), filterData, _raycastObject.shape);
    _raycastObject.rigid->userData = new PhysicsObjectData(PhysicsObjectType::GAME_OBJECT, this);


    // check all this is necessary
    if (_collisionBody) {
        PutRigidBodyToSleep();
        _collisionBody->setGlobalPose(PxTransform(Util::GlmMat4ToPxMat44(_modelMatrix)));
    }
    if (_raycastObject.rigid) {
        _raycastObject.rigid->setGlobalPose(PxTransform(Util::GlmMat4ToPxMat44(_modelMatrix)));
    }
}

void GameObject::SetRaycastShapeFromModel(Model* model) {	
    if (!model) {
		return;
	}
    _raycastObject.model = model;
    UpdateRaycastObject();
}

void GameObject::SetRaycastShape(PxShape* shape) {
	if (!shape) {
		return;
	}
	if (_raycastObject.rigid) {
        _raycastObject.rigid->release();
	}
	PhysicsFilterData filterData;
	filterData.raycastGroup = RAYCAST_ENABLED;
	filterData.collisionGroup = CollisionGroup::NO_COLLISION;
	filterData.collidesWith = CollisionGroup::NO_COLLISION;
    _raycastObject.rigid = Physics::CreateRigidStatic(Transform(), filterData, shape);
    _raycastObject.rigid->userData = new PhysicsObjectData(PhysicsObjectType::GAME_OBJECT, this);

}

void GameObject::SetModelMatrixMode(ModelMatrixMode modelMatrixMode) {
	_modelMatrixMode = modelMatrixMode;
}

void GameObject::SetPhysicsTransform(glm::mat4 worldMatrix) {
	std::cout << Util::Mat4ToString(worldMatrix) << "\n";
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
    if (_raycastObject.rigid) {
        _raycastObject.rigid->release();
    }
    if (_raycastObject.shape) {
        _raycastObject.shape->release();
    }
   /* if (_editorRaycastBody) {
        _editorRaycastBody->release();
    }
    if (_editorRaycastShape) {
        _editorRaycastShape->release();
    }*/
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

    if (_respawns) {
        _collected = true;
        _pickupCoolDownTime = Config::item_respawn_time;
        Transform transform;
        transform.position.y = -100;
        ((PxRigidDynamic*)_collisionBody)->putToSleep();
        _collisionBody->setGlobalPose(PxTransform(Util::GlmMat4ToPxMat44(transform.to_mat4())));
        DisableRaycasting();
        _wasPickedUpLastFrame = true;
    }
    else {
        for (int i = 0; i < Scene::_gameObjects.size(); i++) {
            if (this == &Scene::_gameObjects[i]) {

                // Remove any bullet decals attached to this rigid
                for (int j = 0; j < Scene::_decals.size(); j++) {
                    if (Scene::_decals[j].parent == (PxRigidBody*)_raycastObject.rigid) {                        
                        Scene::_decals.erase(Scene::_decals.begin() + j);
                        j--;
                    }
                }

                // Cleanup
                _collisionBody->release();
                _raycastObject.shape->release();
                _raycastObject.rigid->release();
                Scene::_gameObjects.erase(Scene::_gameObjects.begin() + i);
                break;
            }
        }
    }
}

void GameObject::PutRigidBodyToSleep() {
    if (_collisionBody) {
        ((PxRigidDynamic*)_collisionBody)->putToSleep();
    }
}

void GameObject::SetPickUpType(PickUpType pickupType) {
	_pickupType = pickupType;
}

bool GameObject::IsCollectable() {
	return (_pickupType != PickUpType::NONE);
}

void GameObject::DisableRaycasting() {
    auto filterData = _raycastObject.shape->getQueryFilterData();
    filterData.word0 = RAYCAST_DISABLED;
    _raycastObject.shape->setQueryFilterData(filterData);
}

void GameObject::EnableRaycasting() {

    if (!_raycastObject.shape) {
        std::cout << "there is no raycast shape for game object with name: " << _name << "\n";
        return;
    }

    auto filterData = _raycastObject.shape->getQueryFilterData();
    filterData.word0 = RAYCAST_ENABLED;
    _raycastObject.shape->setQueryFilterData(filterData);
}

bool GameObject::HasMovedSinceLastFrame() {
    return (
        _wasPickedUpLastFrame ||
        _wasRespawnedUpLastFrame ||
        _aabb.position != _aabbPreviousFrame.position && _aabb.extents != _aabbPreviousFrame.extents
    );
}

void GameObject::LoadSavedState() {

    // Collision body
    if (_collisionBody) {
        PxMat44 matrix = Util::GlmMat4ToPxMat44(_transform.to_mat4());
        _collisionBody->setGlobalPose(PxTransform(matrix));
        if (_wakeOnStart) {
            _collisionBody->addForce(PxVec3(0,0,0));
        }
        else {
            ((PxRigidDynamic*)_collisionBody)->putToSleep();
        }
    }

    // Open state
    _openTransform = Transform();

    // Pickup
    if (_pickupType != PickUpType::NONE) {
        _pickupCoolDownTime = 0;
        _wasPickedUpLastFrame = false;
        _collected = false;
    }

}
/*
void GameObject::EnableRaycasting() {
    PxFilterData filterData = _raycastObject.shape->getQueryFilterData();
    filterData.word0 = RAYCAST_ENABLED;
    _raycastObject.shape->setQueryFilterData(filterData);       // ray casts
    _raycastObject.shape->setSimulationFilterData(filterData);  // collisions
}


void GameObject::DisableRaycasting() {
    PxFilterData filterData = _raycastObject.shape->getQueryFilterData();
    filterData.word0 = RAYCAST_DISABLED;
    _raycastObject.shape->setQueryFilterData(filterData);       // ray casts
    _raycastObject.shape->setSimulationFilterData(filterData);  // collisions
}

void GameObject::SetCollisionGroup(CollisionGroup collisionGroup) {
    PxFilterData filterData = _raycastObject.shape->getQueryFilterData();
    filterData.word1 = collisionGroup;
    _raycastObject.shape->setQueryFilterData(filterData);       // ray casts
    _raycastObject.shape->setSimulationFilterData(filterData);  // collisions

}

void GameObject::SetCollidesWithGroup(PxU32 collisionGroup) {
    PxFilterData filterData = _raycastObject.shape->getQueryFilterData();
    filterData.word1 = collisionGroup;
    _raycastObject.shape->setQueryFilterData(filterData);       // ray casts
    _raycastObject.shape->setSimulationFilterData(filterData);  // collisions
}*/
