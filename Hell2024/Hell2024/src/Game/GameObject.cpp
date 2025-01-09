#pragma once
#include "GameObject.h"
#include "Scene.h"
#include "../Input/Input.h"
#include "../Config.hpp"
#include "../BackEnd/BackEnd.h"
#include "../Core/AssetManager.h"

PxFilterData GetPxFilterDataFromCollisionType(CollisionType collisionType) {

    PxFilterData filterData;
    if (collisionType == CollisionType::NONE) {
        filterData.word0 = 0;
        filterData.word1 = 0;
        filterData.word2 = 0;
    }
    else if (collisionType == CollisionType::PICKUP) {
        filterData.word0 = 0;
        filterData.word1 = (PxU32)(CollisionGroup)(GENERIC_BOUNCEABLE);
        filterData.word2 = (PxU32)(CollisionGroup)(GENERIC_BOUNCEABLE | ENVIROMENT_OBSTACLE);
    }
    else if (collisionType == CollisionType::STATIC_ENVIROMENT) {
        filterData.word0 = 0;
        filterData.word1 = (PxU32)(CollisionGroup)(ENVIROMENT_OBSTACLE);
        filterData.word2 = (PxU32)(CollisionGroup)(GENERIC_BOUNCEABLE | BULLET_CASING | PLAYER | RAGDOLL);
    }
    else if (collisionType == CollisionType::STATIC_ENVIROMENT_NO_DOG) {
        filterData.word0 = 0;
        filterData.word1 = (PxU32)(CollisionGroup)(ENVIROMENT_OBSTACLE_NO_DOG);
        filterData.word2 = (PxU32)(CollisionGroup)(GENERIC_BOUNCEABLE | BULLET_CASING | PLAYER | RAGDOLL);
    }
    else if (collisionType == CollisionType::BOUNCEABLE) {
        filterData.word0 = 0;
        filterData.word1 = (PxU32)(CollisionGroup)(GENERIC_BOUNCEABLE);
        filterData.word2 = (PxU32)(CollisionGroup)(GENERIC_BOUNCEABLE | ENVIROMENT_OBSTACLE | RAGDOLL);
    }
    else if (collisionType == CollisionType::BULLET_CASING) {
        filterData.word0 = 0;
        filterData.word1 = (PxU32)(CollisionGroup)(BULLET_CASING);
        filterData.word2 = (PxU32)(CollisionGroup)(ENVIROMENT_OBSTACLE);
    }
    return filterData;
}

void GameObject::DisableShadows() {
    m_castShadows = false;
}

void GameObject::SetCollisionType(CollisionType collisionType) {
    PxFilterData filterData = GetPxFilterDataFromCollisionType(collisionType);
    for (PxShape*& collisionShape : m_collisionRigidBody.GetCollisionShapes()) {
        collisionShape->setQueryFilterData(filterData);       // ray casts
        collisionShape->setSimulationFilterData(filterData);  // collisions
    }
    m_collisionType = collisionType;
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
    if (m_raycastRigidStatic.pxRigidStatic) {
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

    // bullshit from the past
    // bullshit from the past
    // bullshit from the past
    // bullshit from the past
    // bullshit from the past
    // bullshit from the past
    // bullshit from the past


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


    if (_collected) {
        Transform transform;
        transform.position.y = -50;
        ((PxRigidDynamic*)m_collisionRigidBody.pxRigidBody)->putToSleep();
        m_collisionRigidBody.SetGlobalPose(transform.to_mat4());
    }


    if (m_collisionType == CollisionType::PICKUP && !_respawns) {
        m_hackTimer += deltaTime;
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
	if (_modelMatrixMode == ModelMatrixMode::GAME_TRANSFORM) {
		_modelMatrix = GetGameWorldMatrix();
		if (m_collisionRigidBody.Exists()) {
            if (m_collisionRigidBody.kinematic) {
                PutRigidBodyToSleep();
            }
            m_collisionRigidBody.SetGlobalPose(_modelMatrix);
		}
		if (m_raycastRigidStatic.pxRigidStatic) {
            m_raycastRigidStatic.pxRigidStatic->setGlobalPose(PxTransform(Util::GlmMat4ToPxMat44(_modelMatrix)));
		}
	}
    // Has physics object
	else if (_modelMatrixMode == ModelMatrixMode::PHYSX_TRANSFORM && m_collisionRigidBody.Exists()) {
		Transform transform;
        transform.scale = _transform.scale;
        _modelMatrix = m_collisionRigidBody.GetGlobalPoseAsMatrix() * transform.to_mat4();

		if (m_raycastRigidStatic.pxRigidStatic) {
            m_raycastRigidStatic.pxRigidStatic->setGlobalPose(m_collisionRigidBody.GetGlobalPoseAsPxTransform());
		}
	}
    
    UpdatePhysXPointers();

    // AABB
    if (m_raycastRigidStatic.pxRigidStatic) {
        _aabbPreviousFrame = _aabb;
        glm::vec3 extents = Util::PxVec3toGlmVec3(m_raycastRigidStatic.pxRigidStatic->getWorldBounds().getExtents());
        glm::vec3 center = Util::PxVec3toGlmVec3(m_raycastRigidStatic.pxRigidStatic->getWorldBounds().getCenter());
        glm::vec3 minBounds = center - extents;
        glm::vec3 maxBounds = center + extents;
        _aabb = AABB(minBounds, maxBounds);
    }
    else {
        // Rewrite this to work per mesh
        _aabbPreviousFrame = _aabb;
        std::vector<glm::vec3> modelCorners = {
            glm::vec3(model->m_aabbMin.x, model->m_aabbMin.y, model->m_aabbMin.z),
            glm::vec3(model->m_aabbMax.x, model->m_aabbMin.y, model->m_aabbMin.z),
            glm::vec3(model->m_aabbMin.x, model->m_aabbMax.y, model->m_aabbMin.z),
            glm::vec3(model->m_aabbMax.x, model->m_aabbMax.y, model->m_aabbMin.z),
            glm::vec3(model->m_aabbMin.x, model->m_aabbMin.y, model->m_aabbMax.z),
            glm::vec3(model->m_aabbMax.x, model->m_aabbMin.y, model->m_aabbMax.z),
            glm::vec3(model->m_aabbMin.x, model->m_aabbMax.y, model->m_aabbMax.z),
            glm::vec3(model->m_aabbMax.x, model->m_aabbMax.y, model->m_aabbMax.z)
        };
        glm::vec3 worldMinBounds = glm::vec3(std::numeric_limits<float>::max());
        glm::vec3 worldMaxBounds = glm::vec3(std::numeric_limits<float>::lowest());
        for (const auto& corner : modelCorners) {
            glm::vec4 worldCorner = _modelMatrix * glm::vec4(corner, 1.0f);
            glm::vec3 worldCornerVec3(worldCorner);
            worldMinBounds = glm::min(worldMinBounds, worldCornerVec3);
            worldMaxBounds = glm::max(worldMaxBounds, worldCornerVec3);
        }
        _aabb = AABB(worldMinBounds, worldMaxBounds);
    }

    UpdateRenderItems();
}

void GameObject::UpdatePhysXPointers() {
    // Update raycast object PhysX pointer
    if (m_raycastRigidStatic.pxRigidStatic) {
        if (m_raycastRigidStatic.pxRigidStatic->userData) {
            delete static_cast<PhysicsObjectData*>(m_raycastRigidStatic.pxRigidStatic->userData);
        }
        m_raycastRigidStatic.pxRigidStatic->userData = new PhysicsObjectData(ObjectType::GAME_OBJECT, this);
    }
    // Update collision object PhysX pointer
    if (m_collisionRigidBody.Exists()) {
        if (m_collisionRigidBody.pxRigidBody->userData) {
            delete static_cast<PhysicsObjectData*>(m_collisionRigidBody.pxRigidBody->userData);
        }
        m_collisionRigidBody.pxRigidBody->userData = new PhysicsObjectData(ObjectType::GAME_OBJECT, this);
    }
}


void GameObject::SetWakeOnStart(bool value) {
    _wakeOnStart = value;
}

void GameObject::UpdateEditorPhysicsObject() {
	/*if (_editorRaycastBody) {
		if (_model_OLDMatrixMode == ModelMatrixMode::GAME_TRANSFORM) {
			PxQuat quat = Util::GlmQuatToPxQuat(GetWorldRotation());
			PxVec3 position = Util::GlmVec3toPxVec3(GetWorldPosition());
			PxTransform transform = PxTransform(position, quat);
			_editorRaycastBody->setGlobalPose(transform);
		}
		else if (_model_OLDMatrixMode == ModelMatrixMode::PHYSX_TRANSFORM && _collisionBody) {
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

	if (!m_collisionRigidBody.Exists()) {
		return;
	}
	auto flags = m_collisionRigidBody.pxRigidBody->getRigidBodyFlags();
	if (flags & PxRigidBodyFlag::eKINEMATIC) {
		return;
	}
	PxVec3 force = PxVec3(direction.x, direction.y, direction.z) * strength;
    m_collisionRigidBody.pxRigidBody->addForce(force);
}

void GameObject::UpdateRigidBodyMassAndInertia(float density) {
	if (m_collisionRigidBody.pxRigidBody) {
		PxRigidBodyExt::updateMassAndInertia(*m_collisionRigidBody.pxRigidBody, density);
	}
}


void GameObject::MakeGold() {
    m_isGold = true;
}


void GameObject::SetAudioOnOpen(const char* filename, float volume) {
	_audio.onOpen = { filename, volume };
}

glm::vec3 GameObject::GetWorldSpaceOABBCenter() {
	return GetWorldPosition() + _boundingBox.offsetFromModelOrigin;
}

void GameObject::SetAudioOnClose(const char* filename, float volume) {
	_audio.onClose = { filename, volume };
}

void GameObject::SetAudioOnInteract(const char* filename, float volume) {
	_audio.onInteract = { filename, volume };
}

void GameObject::SetOpenState(OpenState openState, float speed, float min, float max) {
	_openState = openState;
	_openSpeed = speed;
	_minOpenAmount = min;
	_maxOpenAmount = max;
}

void GameObject::SetModel(const std::string& name){
    model = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName(name.c_str()));
    if (model) {
        m_meshMaterialIndices.resize(model->GetMeshCount());
        m_meshBlendingModes.resize(model->GetMeshCount());
    }
    else {
        std::cout << "Failed to set model '" << name << "', it does not exist.\n";
    }
    for (BlendingMode& blendingMode : m_meshBlendingModes) {
        blendingMode = BlendingMode::NONE;
    }
}

void GameObject::SetMeshMaterialByMeshName(std::string meshName, const char* materialName) {
	int materialIndex = AssetManager::GetMaterialIndex(materialName);
	if (model && materialIndex != -1) {
        for (int i = 0; i < model->GetMeshCount(); i++) {

            if (AssetManager::GetMeshByIndex(model->GetMeshIndices()[i])->name == meshName) {
            //if (model->GetMeshNameByIndex(i) == meshName) {
				m_meshMaterialIndices[i] = materialIndex;
				return;
			}
		}
	}
	if (!model) {
		std::cout << "Tried to call SetMeshMaterialByMeshName() but this GameObject has a nullptr model\n";
	}
	else if (materialIndex == -1) {
		std::cout << "Tried to call SetMeshMaterialByMeshName() but the material index was -1\n";
	}
    else {
        std::cout << "Tried to call SetMeshMaterialByMeshName() but the meshName '" << meshName << "' not found\n";
    }
}


void GameObject::SetMeshMaterial(const char* name, int meshIndex) {
	// Range checks
    if (m_meshMaterialIndices.empty()) {
        std::cout << "GameObject::SetMeshMaterial() failed: m_meshMaterials was empty!\n";
		return;
	}
	else if (meshIndex != -1 && meshIndex > m_meshMaterialIndices.size()) {
		std::cout << "Failed to set mesh material with mesh Index " << meshIndex << ", GameObject has _meshMaterials with size " << m_meshMaterialIndices.size() << "\n";
		return;
	}
	// Get the material index via the material name
	int materialIndex = AssetManager::GetMaterialIndex(name);
	// Set material for single mesh
	if (meshIndex != -1) {
		m_meshMaterialIndices[meshIndex] = materialIndex;
	}
	// Set material for all mesh
	if (meshIndex == -1) {
		for (int i = 0; i < m_meshMaterialIndices.size(); i++) {
			m_meshMaterialIndices[i] = materialIndex;
		}
	}
}


void GameObject::SetMeshBlendingMode(const char* meshName, BlendingMode blendingMode) {
    // Range checks
    if (m_meshBlendingModes.empty()) {
        std::cout << "GameObject::SetMeshBlendingMode() failed: m_meshBlendingModes was empty!\n";
        return;
    }    
    bool found = false;
    for (int i = 0; i < model->GetMeshIndices().size(); i++) {
        Mesh* mesh = AssetManager::GetMeshByIndex(model->GetMeshIndices()[i]);
        if (mesh && mesh->name == meshName) {
            m_meshBlendingModes[i] = blendingMode;
            found = true;
        }
    }
    if (!found) {
        std::cout << "GameObject::SetMeshBlendingMode() failed: " << meshName << " was not found!\n";
    }
}

void GameObject::SetMeshBlendingModes(BlendingMode blendingMode) {
    for (int i = 0; i < model->GetMeshIndices().size(); i++) {
        Mesh* mesh = AssetManager::GetMeshByIndex(model->GetMeshIndices()[i]);
        if (mesh) {
            m_meshBlendingModes[i] = blendingMode;
        }
    }
}

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

void GameObject::SetKinematic(bool value) {
    if (!m_collisionRigidBody.Exists()) {
        m_collisionRigidBody.CreateRigidBody(_transform.to_mat4());
        m_collisionRigidBody.PutToSleep();
    }
    m_collisionRigidBody.SetKinematic(value);
}

void GameObject::DisableRespawnOnPickup() {
    _respawns = false;
}

void GameObject::AddCollisionShape(PxShape* shape, PhysicsFilterData physicsFilterData) {
    if (!m_collisionRigidBody.Exists()) {
        m_collisionRigidBody.CreateRigidBody(_transform.to_mat4());
        m_collisionRigidBody.PutToSleep();
    }
    m_collisionRigidBody.AddCollisionShape(shape, physicsFilterData);
}

void GameObject::AddCollisionShapeFromModelIndex(unsigned int modelIndex, glm::vec3 scale) {
    m_convexModelIndex = modelIndex;
    if (model && !m_collisionRigidBody.Exists()) {
        m_collisionRigidBody.CreateRigidBody(_transform.to_mat4());
        m_collisionRigidBody.PutToSleep();
    }
    m_collisionRigidBody.AddCollisionShapeFromModelIndex(modelIndex, GetPxFilterDataFromCollisionType(m_collisionType), scale);
}

void GameObject::AddCollisionShapeFromBoundingBox(BoundingBox& boundingBox) {
    if (!m_collisionRigidBody.Exists()) {
        m_collisionRigidBody.CreateRigidBody(_transform.to_mat4());
        m_collisionRigidBody.PutToSleep();
    }
    m_collisionRigidBody.AddCollisionShapeFromBoundingBox(boundingBox, GetPxFilterDataFromCollisionType(m_collisionType));
}

void GameObject::UpdateRigidStatic() {

    // check what calls this and fix the code below
    // check what calls this and fix the code below
    // check what calls this and fix the code below
    // check what calls this and fix the code below

    // actually the funciton below called this. and you've moved this into the SETSHAPE function of the rigidstatic, veryify this by deleting this func
    // actually the funciton below called this. and you've moved this into the SETSHAPE function of the rigidstatic, veryify this by deleting this func
    // actually the funciton below called this. and you've moved this into the SETSHAPE function of the rigidstatic, veryify this by deleting this func
    // actually the funciton below called this. and you've moved this into the SETSHAPE function of the rigidstatic, veryify this by deleting this func


    /*
    // REMOVE THIS! ITS A HACK CAUSE YOU HAVEN'T GOT THIS WORKING WHEN THE SHAPE IS NOT A TRI MESH
    if (model && model->GetName() == "SmallCube") {
        return;
    }

    if (m_raycastRigidStatic.pxRigidStatic) {
        m_raycastRigidStatic.pxRigidStatic->release();
    }
    if (!m_raycastRigidStatic.model->_triangleMesh) {
        m_raycastRigidStatic.model->CreateTriangleMesh();
    }

    PhysicsFilterData filterData;
    filterData.raycastGroup = RAYCAST_ENABLED;
    filterData.collisionGroup = CollisionGroup::NO_COLLISION;
    filterData.collidesWith = CollisionGroup::NO_COLLISION;
    PxShapeFlags shapeFlags(PxShapeFlag::eSCENE_QUERY_SHAPE); // Most importantly NOT eSIMULATION_SHAPE. PhysX does not allow for tri mesh.
    m_raycastRigidStatic.pxShape = Physics::CreateShapeFromTriangleMesh(m_raycastRigidStatic.model->_triangleMesh, shapeFlags, Physics::GetDefaultMaterial(), _transform.scale);
    m_raycastRigidStatic.pxRigidStatic = Physics::CreateRigidStatic(Transform(), filterData, m_raycastRigidStatic.pxShape);
    m_raycastRigidStatic.pxRigidStatic->userData = new PhysicsObjectData(PhysicsObjectType::GAME_OBJECT, this);


    // check all this is necessary
    if (m_collisionRigidBody.Exists()) {
        PutRigidBodyToSleep();
        m_collisionRigidBody.SetGlobalPose(_modelMatrix);
    }
    if (m_raycastRigidStatic.pxRigidStatic) {
        m_raycastRigidStatic.pxRigidStatic->setGlobalPose(PxTransform(Util::GlmMat4ToPxMat44(_modelMatrix)));
    }*/
}

void GameObject::SetRaycastShapeFromModelIndex(unsigned int modelIndex) {
    if (!model) {
		return;
	}
    PxShapeFlags shapeFlags(PxShapeFlag::eSCENE_QUERY_SHAPE);
    PxTriangleMesh* triangleMesh = Physics::CreateTriangleMeshFromModelIndex(modelIndex);
    PxShape* shape = Physics::CreateShapeFromTriangleMesh(triangleMesh, shapeFlags, Physics::GetDefaultMaterial(), _transform.scale);
    m_raycastRigidStatic.SetShape(shape, this);
}

void GameObject::SetRaycastShape(PxShape* shape) {
    m_raycastRigidStatic.Destroy();
	PhysicsFilterData filterData;
	filterData.raycastGroup = RAYCAST_ENABLED;
	filterData.collisionGroup = CollisionGroup::NO_COLLISION;
	filterData.collidesWith = CollisionGroup::NO_COLLISION;
    m_raycastRigidStatic.pxShape = shape;
    m_raycastRigidStatic.pxRigidStatic = Physics::CreateRigidStatic(Transform(), filterData, shape);
    m_raycastRigidStatic.pxRigidStatic->userData = new PhysicsObjectData(ObjectType::GAME_OBJECT, this);

}

void GameObject::SetModelMatrixMode(ModelMatrixMode modelMatrixMode) {
	_modelMatrixMode = modelMatrixMode;
}

void GameObject::SetPhysicsTransform(glm::mat4 worldMatrix) {
	m_collisionRigidBody.SetGlobalPose(worldMatrix);
}

void GameObject::CleanUp() {
    m_collisionRigidBody.Destroy();
    m_raycastRigidStatic.Destroy();
}

std::vector<Vertex> GameObject::GetAABBVertices() {
    glm::vec3 color = YELLOW;
    if (HasMovedSinceLastFrame()) {
        color = RED;
    }
    return Util::GetAABBVertices(_aabb, color);
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
	triA.v0 = pos0;
	triA.v1 = pos1;
	triA.v2 = pos1 + glm::vec3(0, doorHeight, 0);
	//triA.color = YELLOW;
	Triangle triB;
	triB.v0 = pos1 + glm::vec3(0, doorHeight, 0);
	triB.v1 = pos0 + glm::vec3(0, doorHeight, 0);
	triB.v2 = pos0;
	//triB.color = YELLOW;

	// BACK
	Triangle triC;
	triC.v2 = pos2;
	triC.v1 = pos3;
	triC.v0 = pos3 + glm::vec3(0, doorHeight, 0);
	//triC.color = YELLOW;
	Triangle triD;
	triD.v2 = pos3 + glm::vec3(0, doorHeight, 0);
	triD.v1 = pos2 + glm::vec3(0, doorHeight, 0);
	triD.v0 = pos2;
	//triD.color = YELLOW;

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
        transform.position.y = -1;
        ((PxRigidDynamic*)m_collisionRigidBody.pxRigidBody)->putToSleep();
        m_collisionRigidBody.SetGlobalPose(transform.to_mat4());
        DisableRaycasting();
        _wasPickedUpLastFrame = true;
    }
    else {

        for (int i = 0; i < Scene::GetGameObjectCount(); i++) {
            if (this == &Scene::GetGamesObjects()[i]) {

                // you commented this out because your refactoring of gameobject collision objects broke this
                // you commented this out because your refactoring of gameobject collision objects broke this
                // you commented this out because your refactoring of gameobject collision objects broke this
                // Remove any bullet decals attached to this rigid
                /*for (int j = 0; j < Scene::_decals.size(); j++) {
                    if (Scene::_decals[j].parent == (PxRigidBody*)m_raycastRigidStatic.pxRigidStatic) {
                        Scene::_decals.erase(Scene::_decals.begin() + j);
                        j--;
                    }
                }*/

                // Cleanup
                m_raycastRigidStatic.Destroy();
                m_collisionRigidBody.Destroy();
                Scene::GetGamesObjects().erase(Scene::GetGamesObjects().begin() + i);
                break;
            }
        }
    }
}

void GameObject::PutRigidBodyToSleep() {
    m_collisionRigidBody.PutToSleep();
}

void GameObject::SetPickUpType(PickUpType pickupType) {
	_pickupType = pickupType;
}

bool GameObject::IsCollectable() {
	return (_pickupType != PickUpType::NONE);
}

void GameObject::DisableRaycasting() {
    auto filterData = m_raycastRigidStatic.pxShape->getQueryFilterData();
    filterData.word0 = RAYCAST_DISABLED;
    m_raycastRigidStatic.pxShape->setQueryFilterData(filterData);
}

void GameObject::EnableRaycasting() {

    if (!m_raycastRigidStatic.pxShape) {
        std::cout << "there is no raycast shape for game object with name: " << _name << "\n";
        return;
    }

    auto filterData = m_raycastRigidStatic.pxShape->getQueryFilterData();
    filterData.word0 = RAYCAST_ENABLED;
    m_raycastRigidStatic.pxShape->setQueryFilterData(filterData);
}

bool GameObject::HasMovedSinceLastFrame() {

    return (
        _wasPickedUpLastFrame ||
        _wasRespawnedUpLastFrame ||
        _aabb.boundsMin != _aabbPreviousFrame.boundsMin && _aabb.boundsMax != _aabbPreviousFrame.boundsMax && _aabb.GetCenter() != _aabbPreviousFrame.GetCenter() ||
        m_collisionRigidBody.IsInMotion()
    );
}

void GameObject::LoadSavedState() {

    // Collision body
    if (m_collisionRigidBody.Exists()) {
        m_collisionRigidBody.SetGlobalPose(_transform.to_mat4());
        if (_wakeOnStart) {
            PxRigidDynamic* pxRigidDynamic = (PxRigidDynamic*)m_collisionRigidBody.pxRigidBody;
            pxRigidDynamic->setAngularVelocity(PxVec3(0, 0, 0));
            pxRigidDynamic->setLinearVelocity(PxVec3(0, 0, 0));
            pxRigidDynamic->addForce(PxVec3(0, 0, 0));
        }
        else {
            ((PxRigidDynamic*)m_collisionRigidBody.pxRigidBody)->putToSleep();
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

void GameObject::UpdateRenderItems() {
    m_renderItems.clear();
    m_renderItemsBlended.clear();
    m_renderItemsAlphaDiscarded.clear();
    for (int i = 0; i < model->GetMeshIndices().size(); i++) {
        uint32_t& meshIndex = model->GetMeshIndices()[i];
        Mesh* mesh = AssetManager::GetMeshByIndex(meshIndex);
        RenderItem3D renderItem;
        renderItem.vertexOffset = mesh->baseVertex;
        renderItem.indexOffset = mesh->baseIndex;
        renderItem.modelMatrix = GetModelMatrix();
        renderItem.inverseModelMatrix = inverse(renderItem.modelMatrix);
        renderItem.meshIndex = meshIndex;
        Material* material = AssetManager::GetMaterialByIndex(m_meshMaterialIndices[i]);
        if (m_isGold) {
            renderItem.baseColorTextureIndex = AssetManager::GetGoldBaseColorTextureIndex();
            renderItem.rmaTextureIndex = AssetManager::GetGoldRMATextureIndex();
        }
        else {
            renderItem.baseColorTextureIndex = material->_basecolor;
            renderItem.rmaTextureIndex = material->_rma;
        }
        renderItem.normalMapTextureIndex = material->_normal;
        renderItem.castShadow = m_castShadows;
        renderItem.aabbMin = _aabb.boundsMin;
        renderItem.aabbMax = _aabb.boundsMax;
        if (m_meshBlendingModes[i] == BlendingMode::NONE) {
            m_renderItems.push_back(renderItem);
        }
        else if (m_meshBlendingModes[i] == BlendingMode::BLENDED) {
            m_renderItemsBlended.push_back(renderItem);
        }
        else if (m_meshBlendingModes[i] == BlendingMode::ALPHA_DISCARDED) {
            m_renderItemsAlphaDiscarded.push_back(renderItem);
        }   
    }
}

std::vector<RenderItem3D>& GameObject::GetRenderItems() {
    return m_renderItems;
}

std::vector<RenderItem3D>& GameObject::GetRenderItemsBlended() {
    return m_renderItemsBlended;
}

std::vector<RenderItem3D>& GameObject::GetRenderItemsAlphaDiscarded() {
    return m_renderItemsAlphaDiscarded;
}

void GameObject::PrintMeshNames() {
    if (model) {
        std::cout << model->GetName() << "\n";
        for (uint32_t meshIndex : model->GetMeshIndices()) {
            Mesh* mesh = AssetManager::GetMeshByIndex(meshIndex);
            if (mesh) {
                std::cout << "-" << meshIndex << ": " << mesh->name << " " << Util::Vec3ToString(mesh->aabbMin) << " " << Util::Vec3ToString(mesh->aabbMin) << "\n";
            }
        }
    }
}