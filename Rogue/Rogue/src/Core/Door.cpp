#include "Door.h"
#include "../Core/Player.h"
#include "../Core/Audio.hpp"
#include "../Core/AssetManager.h"
#include "../Util.hpp"


Door::Door(glm::vec3 position, float rotation) {
    this->position = position;
    this->rotation = rotation;
}

void Door::Interact() {
    if (state == CLOSED) {
        state = OPENING;
        Audio::PlayAudio("Door_Open.wav", DOOR_VOLUME);
    }
    else if (state == OPEN) {
        state = CLOSING;
        Audio::PlayAudio("Door_Open.wav", DOOR_VOLUME);
    }
}

void Door::Update(float deltaTime) {
    float openSpeed = 5.208f;
    if (state == OPENING) {
        openRotation -= openSpeed * deltaTime;
        if (openRotation < -1.9f) {
            openRotation = -1.9f;
            state = OPEN;
        }
    }
    if (state == CLOSING) {
        openRotation += openSpeed * deltaTime;
        if (openRotation > 0) {
            openRotation = 0;
            state = CLOSED;
        }
    }

    glm::mat4 modelMatrix = GetDoorModelMatrix();
    if (collisionBody) {
        Transform offset;
        offset.position.z = DOOR_WIDTH * -0.5f;
        offset.position.y = DOOR_HEIGHT * 0.5f;
        offset.position.x = DOOR_EDITOR_DEPTH * -0.5f;
        PxMat44 m = Util::GlmMat4ToPxMat44(modelMatrix * offset.to_mat4());
        PxTransform transform = PxTransform(m);
        collisionBody->setGlobalPose(transform);
    }
    if (raycastBody) {
        PxMat44 m2 = Util::GlmMat4ToPxMat44(modelMatrix);
        PxTransform transform2 = PxTransform(m2);
        raycastBody->setGlobalPose(transform2);
    }
}

void Door::CleanUp() {
    if (collisionBody) {
        collisionBody->release();
    }
    if (collisionShape) {
        collisionShape->release();
    }
    if (raycastBody) {
        raycastBody->release();
    }
    if (raycastShape) {
        raycastShape->release();
    }
}

glm::mat4 Door::GetFrameModelMatrix() {
    Transform frameTransform;
    frameTransform.position = position;
    frameTransform.rotation.y = rotation;
    return frameTransform.to_mat4();
}

glm::mat4 Door::GetDoorModelMatrix() {
    Transform doorTransform;
    doorTransform.position = glm::vec3(0.058520, 0, 0.39550f);
    doorTransform.rotation.y = openRotation; 
    return GetFrameModelMatrix() * doorTransform.to_mat4();
}

glm::vec3 Door::GetVertFrontLeftForEditor(float padding) {
    return GetFrameModelMatrix() * glm::vec4(DOOR_EDITOR_DEPTH + padding, 0, (-DOOR_WIDTH / 2), 1.0f);
}

glm::vec3 Door::GetVertFrontRightForEditor(float padding) {
    return GetFrameModelMatrix() * glm::vec4(DOOR_EDITOR_DEPTH + padding, 0, (DOOR_WIDTH / 2), 1.0f);
}

glm::vec3 Door::GetVertBackLeftForEditor(float padding) {
    return GetFrameModelMatrix() * glm::vec4(-DOOR_EDITOR_DEPTH - padding, 0, (DOOR_WIDTH / 2), 1.0f);
}

glm::vec3 Door::GetVertBackRightForEditor(float padding) {
    return GetFrameModelMatrix() * glm::vec4(-DOOR_EDITOR_DEPTH - padding, 0, (-DOOR_WIDTH / 2), 1.0f);
}

glm::vec3 Door::GetFrontLeftCorner() {
    return GetDoorModelMatrix() * glm::vec4(0, 0, -DOOR_WIDTH, 1.0f);
}

glm::vec3 Door::GetFrontRightCorner() {
    return GetDoorModelMatrix() * glm::vec4(0, 0, 0, 1.0f);
}

glm::vec3 Door::GetBackLeftCorner() {
    return GetDoorModelMatrix() * glm::vec4(-DOOR_EDITOR_DEPTH, 0, -DOOR_WIDTH, 1.0f);
}

glm::vec3 Door::GetBackRightCorner() {
    return GetDoorModelMatrix() * glm::vec4(-DOOR_EDITOR_DEPTH, 0, 0, 1.0f);
}

bool Door::IsInteractable(glm::vec3 playerPosition) {
    float distSqrd = Util::DistanceSquared(position, playerPosition);
    return distSqrd < (INTERACT_DISTANCE * INTERACT_DISTANCE);
}

void Door::CreatePhysicsObject() {

    Model* model = AssetManager::GetModel("Door");
    if (!model) {
        std::cout << "Failed to create door physics object, cause could not find 'Door' model\n";
        return;
    }
    Mesh* mesh = &model->_meshes[3];
    if (!mesh->_triangleMesh) {
        mesh->CreateTriangleMesh();
    }

	if (collisionBody) {
		collisionBody->release();
	}
	if (raycastBody) {
		raycastBody->release();
	}
	if (collisionShape) {
		collisionShape->release();
	}
	if (raycastShape) {
        raycastShape->release();
	}

    PhysicsFilterData filterData;
    filterData.raycastGroup = RAYCAST_DISABLED;
    filterData.collisionGroup = CollisionGroup::ENVIROMENT_OBSTACLE;
    filterData.collidesWith = (CollisionGroup)(GENERIC_BOUNCEABLE | BULLET_CASING);
    collisionShape = Physics::CreateBoxShape(DOOR_EDITOR_DEPTH * 0.5f, DOOR_HEIGHT * 0.5f, DOOR_WIDTH * 0.5f);
    collisionBody = Physics::CreateRigidStatic(Transform(), filterData, collisionShape);

	PxShapeFlags shapeFlags(PxShapeFlag::eSCENE_QUERY_SHAPE); // Most importantly NOT eSIMULATION_SHAPE. PhysX does not allow for tri mesh.
    PhysicsFilterData filterData2;
    filterData2.raycastGroup = RaycastGroup::RAYCAST_ENABLED;
    filterData2.collisionGroup = NO_COLLISION;
    filterData2.collidesWith = NO_COLLISION;
    raycastShape = Physics::CreateShapeFromTriangleMesh(mesh->_triangleMesh, shapeFlags);
    raycastBody = Physics::CreateRigidStatic(Transform(), filterData2, raycastShape);

    PhysicsObjectData* physicsObjectData = new PhysicsObjectData(PhysicsObjectType::DOOR, this);
    raycastBody->userData = physicsObjectData;
}
/*

void Door::InitPxTriangleMesh() {
    std::vector<PxVec3> vertices;
    std::vector<unsigned int> indices;

    Model& model = AssetManager::GetModel("Door");
    for (auto& mesh : model._meshes) {
        for (auto& vertex : mesh.vertices) {
            vertices.push_back(PxVec3(vertex.position.x, vertex.position.y, vertex.position.z));
        }
        for (auto& index : mesh.indices) {
            indices.push_back(index);
        }
    }
    s_triangleMesh = Physics::CreateTriangleMesh(vertices.size(), vertices.data(), indices.size() / 3, indices.data());
}
*/