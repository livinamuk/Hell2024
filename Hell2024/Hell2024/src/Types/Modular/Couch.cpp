#include "Couch.h"
#include "../../Game/Scene.h"

void Couch::Init(CouchCreateInfo createInfo) {
    m_transform.position = createInfo.position;
    m_transform.rotation.y = createInfo.rotation;

    float cushionHeight = 0.555f;
    Transform shapeOffset;
    shapeOffset.position.y = cushionHeight * 0.5f;
    shapeOffset.position.z = 0.5f;
    PxShape* sofaShapeBigCube = Physics::CreateBoxShape(1, cushionHeight * 0.5f, 0.4f, shapeOffset);

    PhysicsFilterData genericObstacleFilterData;
    genericObstacleFilterData.raycastGroup = RAYCAST_DISABLED;
    genericObstacleFilterData.collisionGroup = CollisionGroup::ENVIROMENT_OBSTACLE;
    genericObstacleFilterData.collidesWith = (CollisionGroup)(GENERIC_BOUNCEABLE | BULLET_CASING | PLAYER | RAGDOLL);

    PhysicsFilterData cushionFilterData;
    cushionFilterData.raycastGroup = RAYCAST_DISABLED;
    cushionFilterData.collisionGroup = CollisionGroup::GENERIC_BOUNCEABLE;
    cushionFilterData.collidesWith = CollisionGroup(ENVIROMENT_OBSTACLE | GENERIC_BOUNCEABLE);
    float cushionDensity = 20.0f;

    GameObject* sofaGameObject = Scene::GetGameObjectByIndex(m_sofaGameObjectIndex);
    sofaGameObject->SetPosition(m_transform.position);
    sofaGameObject->SetRotationY(m_transform.rotation.y);
    sofaGameObject->SetName("Sofa");
    sofaGameObject->SetModel("Sofa_Cushionless");
    sofaGameObject->SetMeshMaterial("Sofa");
    sofaGameObject->SetKinematic(true);
    sofaGameObject->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("Sofa_Cushionless"));
    sofaGameObject->AddCollisionShape(sofaShapeBigCube, genericObstacleFilterData);
    sofaGameObject->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaBack_ConvexMesh"));
    sofaGameObject->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaLeftArm_ConvexMesh"));
    sofaGameObject->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaRightArm_ConvexMesh"));
    sofaGameObject->SetModelMatrixMode(ModelMatrixMode::GAME_TRANSFORM);
    sofaGameObject->SetCollisionType(CollisionType::STATIC_ENVIROMENT);

    GameObject* cushion0 = Scene::GetGameObjectByIndex(m_cusionGameObjectIndices[0]);
    cushion0->SetPosition(m_transform.position);
    cushion0->SetRotationY(m_transform.rotation.y);
    cushion0->SetModel("SofaCushion0");
    cushion0->SetMeshMaterial("Sofa");
    cushion0->SetName("SofaCushion0");
    cushion0->SetKinematic(false);
    cushion0->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaCushion0"));
    cushion0->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaCushion0_ConvexMesh"));
    cushion0->SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
    cushion0->UpdateRigidBodyMassAndInertia(cushionDensity);
    cushion0->SetCollisionType(CollisionType::BOUNCEABLE);

    GameObject* cushion1 = Scene::GetGameObjectByIndex(m_cusionGameObjectIndices[1]);
    cushion1->SetPosition(m_transform.position);
    cushion1->SetRotationY(m_transform.rotation.y);
    cushion1->SetModel("SofaCushion1");
    cushion1->SetName("SofaCushion1");
    cushion1->SetMeshMaterial("Sofa");
    cushion1->SetKinematic(false);
    cushion1->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaCushion0"));
    cushion1->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaCushion1_ConvexMesh"));
    cushion1->SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
    cushion1->UpdateRigidBodyMassAndInertia(cushionDensity);
    cushion1->SetCollisionType(CollisionType::BOUNCEABLE);

    GameObject* cushion2 = Scene::GetGameObjectByIndex(m_cusionGameObjectIndices[2]);
    cushion2->SetPosition(m_transform.position);
    cushion2->SetRotationY(m_transform.rotation.y);
    cushion2->SetModel("SofaCushion2");
    cushion2->SetName("SofaCushion2");
    cushion2->SetMeshMaterial("Sofa");
    cushion2->SetKinematic(false);
    cushion2->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaCushion2"));
    cushion2->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaCushion2_ConvexMesh"));
    cushion2->SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
    cushion2->UpdateRigidBodyMassAndInertia(cushionDensity);
    cushion2->SetCollisionType(CollisionType::BOUNCEABLE);

    GameObject* cushion3 = Scene::GetGameObjectByIndex(m_cusionGameObjectIndices[3]);
    cushion3->SetPosition(m_transform.position);
    cushion3->SetRotationY(m_transform.rotation.y);
    cushion3->SetModel("SofaCushion3");
    cushion3->SetName("SofaCushion3");
    cushion3->SetMeshMaterial("Sofa");
    cushion3->SetKinematic(false);
    cushion3->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaCushion3"));
    cushion3->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaCushion3_ConvexMesh"));
    cushion3->SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
    cushion3->UpdateRigidBodyMassAndInertia(cushionDensity);
    cushion3->SetCollisionType(CollisionType::BOUNCEABLE);

    GameObject* cushion4 = Scene::GetGameObjectByIndex(m_cusionGameObjectIndices[4]);
    cushion4->SetPosition(m_transform.position);
    cushion4->SetRotationY(m_transform.rotation.y);
    cushion4->SetModel("SofaCushion4");
    cushion4->SetName("SofaCushion4");
    cushion4->SetMeshMaterial("Sofa");
    cushion4->SetKinematic(false);
    cushion4->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaCushion4"));
    cushion4->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("SofaCushion4_ConvexMesh"));
    cushion4->SetModelMatrixMode(ModelMatrixMode::PHYSX_TRANSFORM);
    cushion4->UpdateRigidBodyMassAndInertia(15.0f);
    cushion4->SetCollisionType(CollisionType::BOUNCEABLE);
}

void Couch::CleanUp() {
    GameObject* sofaGameObject = Scene::GetGameObjectByIndex(m_sofaGameObjectIndex);
    if (sofaGameObject) {
        sofaGameObject->CleanUp();
    }
    for (int i = 0; i < 5; i++) {
        GameObject* cusionGameObject = Scene::GetGameObjectByIndex(m_cusionGameObjectIndices[i]);
        if (cusionGameObject) {
            cusionGameObject->CleanUp();
        }
    }
    m_sofaGameObjectIndex = -1;
    m_cusionGameObjectIndices[0] = -1;
    m_cusionGameObjectIndices[1] = -1;
    m_cusionGameObjectIndices[2] = -1;
    m_cusionGameObjectIndices[3] = -1;
    m_cusionGameObjectIndices[4] = -1;
}

void Couch::SetPosition(glm::vec3 position) {
    m_transform.position = position;
}

void Couch::SetRotationY(float rotation) {
    m_transform.rotation.y = rotation;
}

const glm::vec3 Couch::GetPosition() {
    return m_transform.position;
}

const float Couch::GetRotationY() {
    return m_transform.rotation.y;
}

const glm::mat4 Couch::GetModelMatrix() {
    return m_transform.to_mat4();
}