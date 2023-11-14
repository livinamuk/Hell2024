#pragma once
#include "GameObject.h"
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
	// If this is an item that has been collected, move if way below the world.
	if (_collected) {
		Transform t;
		t.position.y = -100;
		return t.to_mat4();
	}

	GameObject* parent = Scene::GetGameObjectByName(_parentName);
	if (parent) {
		//if (_overrideTransformWithMatrix) {
		//	return parent->GetModelMatrix() * _modelMatrixTransformOverride;
		//}
		//else {
			return parent->GetModelMatrix() * _transform.to_mat4() * _openTransform.to_mat4();
		//}
	}
	//if (_overrideTransformWithMatrix) {
	//	return _modelMatrixTransformOverride;
	//}
	//else {
		return _transform.to_mat4();
	//}
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

void GameObject::SetScriptName(std::string name) {
	_scriptName = name;
}
bool GameObject::IsInteractable() {

	if (_name == "ToiletLid" && Scene::GetGameObjectByName("ToiletSeat")->GetOpenState() == OpenState::OPEN)
		return false;
	if (_name == "ToiletSeat" && Scene::GetGameObjectByName("ToiletLid")->GetOpenState() == OpenState::OPEN)
		return false;

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
	// Interact text
	/*else if (_interactType == InteractType::TEXT) {
		// blit any text
		TextBlitter::Type(_interactText);
		// call any callback
		if (_interactCallback)
			_interactCallback();
		// Audio
		if (_audio.onInteract.filename != "") {
			Audio::PlayAudio(_audio.onInteract);
		}
		// Default
		else if (_interactText.length() > 0) {
			Audio::PlayAudio("RE_type.wav", 1.0f);
		}
	}
	// Question text
	else if (_interactType == InteractType::QUESTION) {
		TextBlitter::AskQuestion(_interactText, this->_interactCallback, nullptr);
		Audio::PlayAudio("RE_type.wav", 1.0f);
	}
	// Pick up
	else if (_interactType == InteractType::PICKUP) {
		TextBlitter::AskQuestion(_interactText, this->_interactCallback, this);
		Audio::PlayAudio("RE_type.wav", 1.0f);
	}
	// Callback only
	else if (_interactType == InteractType::CALLBACK_ONLY) {
		_interactCallback();
	}*/
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