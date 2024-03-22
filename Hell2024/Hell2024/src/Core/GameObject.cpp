#pragma once
#include "GameObject.h"
#include "Scene.h"
#include "Config.hpp"
#include "Input.h"
#include "../EngineState.hpp"
#include "../API/OpenGL/GL_assetManager.h"
#include "../Core/AssetManager.h"

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


void GameObject::SetScale(glm::vec3 scale) {
	_transform.scale = glm::vec3(scale);

    // Scale physx objects
    if (raycastRigidStatic.pxRigidStatic) {
        // commented out below coz it fucked the PhysX pointers of everything
        //UpdateRigidStatic();
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

    // DEAL WITH THIS A BETTER WAY
    // DEAL WITH THIS A BETTER WAY
    // DEAL WITH THIS A BETTER WAY
    // DEAL WITH THIS A BETTER WAY
    // DEAL WITH THIS A BETTER WAY
    // DEAL WITH THIS A BETTER WAY

    if (_name == "GlockAmmo_PickUp") {
           
        GameObject* topDraw = Scene::GetGameObjectByName("TopDraw");
        if (topDraw) {

            glm::mat4 globalPose = collisionRigidBody.GetGlobalPoseAsMatrix();        
            float width = 0.4f;
            float height = 0.4f;
            float depth = 0.4f;
            PxShape* overlapShape = Physics::CreateBoxShape(width, height, depth, Transform());
            const PxGeometry& overlapGeometry = overlapShape->getGeometry();
            const PxTransform shapePose = collisionRigidBody.GetGlobalPoseAsPxTransform();
            OverlapReport overlapReport = Physics::OverlapTest(overlapGeometry, shapePose, CollisionGroup::GENERIC_BOUNCEABLE);

            if (overlapReport.HitsFound()) {
                for (auto& hit : overlapReport.hits) {

                    if (hit == topDraw->collisionRigidBody.pxRigidBody) {
                        float speed = 3.0f;
                        Transform displacement;

                        if (topDraw->_openState == OpenState::OPENING) {
                            displacement.position.z += deltaTime * speed;
                        }
                        else if (topDraw->_openState == OpenState::CLOSING) {
                            displacement.position.z -= deltaTime * speed;
                        }
                        collisionRigidBody.SetGlobalPose(globalPose* displacement.to_mat4());
                    }
                }                
            }
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

        float maxOpenValue = -(NOOSE_PI * 0.5f) - 0.12f;
        float maxOpenValueSeat = (NOOSE_PI * 0.5f) + 0.12f;
        float speed = 12.0f;
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
		if (collisionRigidBody.Exists()) {
            if (collisionRigidBody.kinematic) {
                PutRigidBodyToSleep();
            }
            collisionRigidBody.SetGlobalPose(_modelMatrix);
		}
		if (raycastRigidStatic.pxRigidStatic) {
            raycastRigidStatic.pxRigidStatic->setGlobalPose(PxTransform(Util::GlmMat4ToPxMat44(_modelMatrix)));
		}
	}
    // Has physics object
	else if (_modelMatrixMode == ModelMatrixMode::PHYSX_TRANSFORM && collisionRigidBody.Exists()) {
		Transform transform;
        transform.scale = _transform.scale;
        _modelMatrix = collisionRigidBody.GetGlobalPoseAsMatrix() * transform.to_mat4();

		if (raycastRigidStatic.pxRigidStatic) {
            raycastRigidStatic.pxRigidStatic->setGlobalPose(collisionRigidBody.GetGlobalPoseAsPxTransform());
		}
	}

	// Update raycast object PhysX pointer
	if (raycastRigidStatic.pxRigidStatic) {
		if (raycastRigidStatic.pxRigidStatic->userData) {
			delete raycastRigidStatic.pxRigidStatic->userData;
		}
        raycastRigidStatic.pxRigidStatic->userData = new PhysicsObjectData(PhysicsObjectType::GAME_OBJECT, this);
	}
	// Update collision object PhysX pointer
	if (collisionRigidBody.Exists()) {
		if (collisionRigidBody.pxRigidBody->userData) {
			delete collisionRigidBody.pxRigidBody->userData;
		}
        collisionRigidBody.pxRigidBody->userData = new PhysicsObjectData(PhysicsObjectType::GAME_OBJECT, this);
	}

    // AABB
    if (raycastRigidStatic.pxRigidStatic) {
        _aabbPreviousFrame = _aabb;
        _aabb.extents = Util::PxVec3toGlmVec3(raycastRigidStatic.pxRigidStatic->getWorldBounds().getExtents());
        _aabb.position = Util::PxVec3toGlmVec3(raycastRigidStatic.pxRigidStatic->getWorldBounds().getCenter());
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
		result = _transform.to_mat4() * _openTransform.to_mat4();
	}
	return result;
}

void GameObject::AddForceToCollisionObject(glm::vec3 direction, float strength) {
	if (!collisionRigidBody.Exists()) {
		return;
	}
	auto flags = collisionRigidBody.pxRigidBody->getRigidBodyFlags();
	if (flags & PxRigidBodyFlag::eKINEMATIC) {
		return;
	}
	PxVec3 force = PxVec3(direction.x, direction.y, direction.z) * strength; 
    collisionRigidBody.pxRigidBody->addForce(force);
}

void GameObject::UpdateRigidBodyMassAndInertia(float density) {
	if (collisionRigidBody.pxRigidBody) {
		PxRigidBodyExt::updateMassAndInertia(*collisionRigidBody.pxRigidBody, density);
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
	_model = OpenGLAssetManager::GetModel(name);

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

void GameObject::SetTransform(Transform& transform) {
	_transform = transform;
}

void GameObject::SetInteractToAffectAnotherObject(std::string objectName)
{
	//_interactAffectsThisObjectInstead = objectName;
}

void GameObject::SetKinematic(bool value) {
    if (!collisionRigidBody.Exists()) {
        collisionRigidBody.CreateRigidBody(_transform.to_mat4());
        collisionRigidBody.PutToSleep();
    }
    collisionRigidBody.SetKinematic(value);
}

void GameObject::DisableRespawnOnPickup() {
    _respawns = false;
}

void GameObject::AddCollisionShape(PxShape* shape, PhysicsFilterData physicsFilterData) {
    if (!collisionRigidBody.Exists()) {
        collisionRigidBody.CreateRigidBody(_transform.to_mat4());
        collisionRigidBody.PutToSleep();
    }
    collisionRigidBody.AddCollisionShape(shape, physicsFilterData);
}

void GameObject::AddCollisionShapeFromConvexMesh(OpenGLMesh* mesh, PhysicsFilterData physicsFilterData, glm::vec3 scale) {
    if (!collisionRigidBody.Exists()) {
        collisionRigidBody.CreateRigidBody(_transform.to_mat4());
        collisionRigidBody.PutToSleep();
    }
    collisionRigidBody.AddCollisionShapeFromConvexMesh(mesh, physicsFilterData, scale);
}

void GameObject::AddCollisionShapeFromBoundingBox(BoundingBox& boundingBox, PhysicsFilterData physicsFilterData) {
    if (!collisionRigidBody.Exists()) {
        collisionRigidBody.CreateRigidBody(_transform.to_mat4());
        collisionRigidBody.PutToSleep();
    }
    collisionRigidBody.AddCollisionShapeFromBoundingBox(boundingBox, physicsFilterData);
}

void GameObject::SetRaycastShapeFromMesh(OpenGLMesh* mesh) {
	if (!mesh) {
		return;
	}
	if (raycastRigidStatic.pxRigidStatic) {
        raycastRigidStatic.pxRigidStatic->release();
	}
	if (!mesh->_triangleMesh) {
		mesh->CreateTriangleMesh();
	}
	PhysicsFilterData filterData;
	filterData.raycastGroup = RAYCAST_ENABLED;
	filterData.collisionGroup = CollisionGroup::NO_COLLISION;
	filterData.collidesWith = CollisionGroup::NO_COLLISION;
	PxShapeFlags shapeFlags(PxShapeFlag::eSCENE_QUERY_SHAPE); // Most importantly NOT eSIMULATION_SHAPE. PhysX does not allow for tri mesh.
    raycastRigidStatic.pxShape = Physics::CreateShapeFromTriangleMesh(mesh->_triangleMesh, shapeFlags);
    raycastRigidStatic.pxRigidStatic = Physics::CreateRigidStatic(Transform(), filterData, raycastRigidStatic.pxShape);
    raycastRigidStatic.pxRigidStatic->userData = new PhysicsObjectData(PhysicsObjectType::GAME_OBJECT, this);
}

void GameObject::UpdateRigidStatic() {

    // REMOVE THIS! ITS A HACK CAUSE YOU HAVEN'T GOT THIS WORKING WHEN THE SHAPE IS NOT A TRI MESH
    if (_model && _model->_name == "SmallCube") {
        return;
    }

    if (raycastRigidStatic.pxRigidStatic) {
        raycastRigidStatic.pxRigidStatic->release();
    }
    if (!raycastRigidStatic.model->_triangleMesh) {
        raycastRigidStatic.model->CreateTriangleMesh();
    }
    
    PhysicsFilterData filterData;
    filterData.raycastGroup = RAYCAST_ENABLED;
    filterData.collisionGroup = CollisionGroup::NO_COLLISION;
    filterData.collidesWith = CollisionGroup::NO_COLLISION;
    PxShapeFlags shapeFlags(PxShapeFlag::eSCENE_QUERY_SHAPE); // Most importantly NOT eSIMULATION_SHAPE. PhysX does not allow for tri mesh.
    raycastRigidStatic.pxShape = Physics::CreateShapeFromTriangleMesh(raycastRigidStatic.model->_triangleMesh, shapeFlags, Physics::GetDefaultMaterial(), _transform.scale);
    raycastRigidStatic.pxRigidStatic = Physics::CreateRigidStatic(Transform(), filterData, raycastRigidStatic.pxShape);
    raycastRigidStatic.pxRigidStatic->userData = new PhysicsObjectData(PhysicsObjectType::GAME_OBJECT, this);


    // check all this is necessary
    if (collisionRigidBody.Exists()) {
        PutRigidBodyToSleep();
        collisionRigidBody.SetGlobalPose(_modelMatrix);
    }
    if (raycastRigidStatic.pxRigidStatic) {
        raycastRigidStatic.pxRigidStatic->setGlobalPose(PxTransform(Util::GlmMat4ToPxMat44(_modelMatrix)));
    }
}

void GameObject::SetRaycastShapeFromModel(OpenGLModel* model) {	
    if (!model) {
		return;
	}
    raycastRigidStatic.model = model;
    UpdateRigidStatic();
}

void GameObject::SetRaycastShape(PxShape* shape) {
	if (!shape) {
		return;
	}
	if (raycastRigidStatic.pxRigidStatic) {
        raycastRigidStatic.pxRigidStatic->release();
	}
	PhysicsFilterData filterData;
	filterData.raycastGroup = RAYCAST_ENABLED;
	filterData.collisionGroup = CollisionGroup::NO_COLLISION;
	filterData.collidesWith = CollisionGroup::NO_COLLISION;
    raycastRigidStatic.pxRigidStatic = Physics::CreateRigidStatic(Transform(), filterData, shape);
    raycastRigidStatic.pxRigidStatic->userData = new PhysicsObjectData(PhysicsObjectType::GAME_OBJECT, this);

}

void GameObject::SetModelMatrixMode(ModelMatrixMode modelMatrixMode) {
	_modelMatrixMode = modelMatrixMode;
}

void GameObject::SetPhysicsTransform(glm::mat4 worldMatrix) {
	collisionRigidBody.SetGlobalPose(worldMatrix);
}

void GameObject::CleanUp() {
    collisionRigidBody.CleanUp();
    raycastRigidStatic.CleanUp();
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
        ((PxRigidDynamic*)collisionRigidBody.pxRigidBody)->putToSleep();
        collisionRigidBody.SetGlobalPose(transform.to_mat4());
        DisableRaycasting();
        _wasPickedUpLastFrame = true;
    }
    else {
        
        for (int i = 0; i < Scene::_gameObjects.size(); i++) {
            if (this == &Scene::_gameObjects[i]) {

                // you commented this out because your refactoring of gameobject collision objects broke this
                // you commented this out because your refactoring of gameobject collision objects broke this
                // you commented this out because your refactoring of gameobject collision objects broke this
                // Remove any bullet decals attached to this rigid
                /*for (int j = 0; j < Scene::_decals.size(); j++) {
                    if (Scene::_decals[j].parent == (PxRigidBody*)raycastRigidStatic.pxRigidStatic) {
                        Scene::_decals.erase(Scene::_decals.begin() + j);
                        j--;
                    }
                }*/

                // Cleanup
                raycastRigidStatic.CleanUp();
                collisionRigidBody.CleanUp();
                Scene::_gameObjects.erase(Scene::_gameObjects.begin() + i);
                break;
            }
        }
    }
}

void GameObject::PutRigidBodyToSleep() {
    collisionRigidBody.PutToSleep();
}

void GameObject::SetPickUpType(PickUpType pickupType) {
	_pickupType = pickupType;
}

bool GameObject::IsCollectable() {
	return (_pickupType != PickUpType::NONE);
}

void GameObject::DisableRaycasting() {
    auto filterData = raycastRigidStatic.pxShape->getQueryFilterData();
    filterData.word0 = RAYCAST_DISABLED;
    raycastRigidStatic.pxShape->setQueryFilterData(filterData);
}

void GameObject::EnableRaycasting() {

    if (!raycastRigidStatic.pxShape) {
        std::cout << "there is no raycast shape for game object with name: " << _name << "\n";
        return;
    }

    auto filterData = raycastRigidStatic.pxShape->getQueryFilterData();
    filterData.word0 = RAYCAST_ENABLED;
    raycastRigidStatic.pxShape->setQueryFilterData(filterData);
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
    if (collisionRigidBody.Exists()) {
        collisionRigidBody.SetGlobalPose(_transform.to_mat4());
        if (_wakeOnStart) {
            collisionRigidBody.pxRigidBody->addForce(PxVec3(0,0,0));
        }
        else {
            ((PxRigidDynamic*)collisionRigidBody.pxRigidBody)->putToSleep();
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
    PxFilterData filterData = raycastRigidStatic.shape->getQueryFilterData();
    filterData.word0 = RAYCAST_ENABLED;
    raycastRigidStatic.shape->setQueryFilterData(filterData);       // ray casts
    raycastRigidStatic.shape->setSimulationFilterData(filterData);  // collisions
}


void GameObject::DisableRaycasting() {
    PxFilterData filterData = raycastRigidStatic.shape->getQueryFilterData();
    filterData.word0 = RAYCAST_DISABLED;
    raycastRigidStatic.shape->setQueryFilterData(filterData);       // ray casts
    raycastRigidStatic.shape->setSimulationFilterData(filterData);  // collisions
}

void GameObject::SetCollisionGroup(CollisionGroup collisionGroup) {
    PxFilterData filterData = raycastRigidStatic.shape->getQueryFilterData();
    filterData.word1 = collisionGroup;
    raycastRigidStatic.shape->setQueryFilterData(filterData);       // ray casts
    raycastRigidStatic.shape->setSimulationFilterData(filterData);  // collisions

}

void GameObject::SetCollidesWithGroup(PxU32 collisionGroup) {
    PxFilterData filterData = raycastRigidStatic.shape->getQueryFilterData();
    filterData.word1 = collisionGroup;
    raycastRigidStatic.shape->setQueryFilterData(filterData);       // ray casts
    raycastRigidStatic.shape->setSimulationFilterData(filterData);  // collisions
}*/
