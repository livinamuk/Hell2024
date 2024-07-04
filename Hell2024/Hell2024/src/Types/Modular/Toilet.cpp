#pragma once
#include "Toilet.h"
#include "../../Game/Scene.h"

Toilet::Toilet(glm::vec3 position, float rotation) {

    transform.position = position;
    transform.rotation.y = rotation;

    Scene::CreateGameObject();
    GameObject* mainBody = Scene::GetGameObjectByIndex(Scene::GetGameObjectCount() - 1);
    mainBody->SetPosition(position);
    mainBody->SetModel("Toilet");
    mainBody->SetName("Toilet");
    mainBody->SetMeshMaterial("Toilet");
    mainBody->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("Toilet"));
    mainBody->SetRotationY(rotation);
    mainBody->SetKinematic(true);
    mainBody->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("Toilet_ConvexMesh"));
    mainBody->AddCollisionShapeFromModelIndex(AssetManager::GetModelIndexByName("Toilet_ConvexMesh1"));
    mainBody->SetModelMatrixMode(ModelMatrixMode::GAME_TRANSFORM);
    mainBody->SetCollisionType(CollisionType::STATIC_ENVIROMENT);

    glm::vec4 seatPosition = mainBody->_transform.to_mat4() * glm::vec4(0.0f, 0.40727f, -0.2014f, 1.0f);
    glm::vec4 lidPosition = mainBody->_transform.to_mat4() * glm::vec4(0.0f, 0.40727f, -0.2014f, 1.0f);

    Scene::CreateGameObject();
    GameObject* seat = Scene::GetGameObjectByIndex(Scene::GetGameObjectCount() - 1);
    seat->SetModel("ToiletSeat");
    seat->SetPosition(seatPosition);
    seat->SetRotationY(rotation);
    seat->SetName("ToiletSeat");
    seat->SetMeshMaterial("Toilet");
    seat->SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f);
    seat->SetOpenAxis(OpenAxis::ROTATION_NEG_X);
    seat->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("ToiletSeat"));
    seat->SetModelMatrixMode(ModelMatrixMode::GAME_TRANSFORM);

    Scene::CreateGameObject();
    GameObject* lid = Scene::GetGameObjectByIndex(Scene::GetGameObjectCount() - 1);
    lid->SetPosition(lidPosition);
    lid->SetRotationY(rotation);
    lid->SetModel("ToiletLid");
    lid->SetName("ToiletLid");
    lid->SetMeshMaterial("Toilet");
    lid->SetOpenState(OpenState::CLOSED, 2.183f, 0, 0.2f);
    lid->SetOpenAxis(OpenAxis::ROTATION_POS_X);
    lid->SetRaycastShapeFromModelIndex(AssetManager::GetModelIndexByName("ToiletLid"));
    lid->SetModelMatrixMode(ModelMatrixMode::GAME_TRANSFORM);
}

void Toilet::Draw(Shader& shader, bool bindMaterial) {
}

void Toilet::Update(float deltaTime) {
}

glm::mat4 Toilet::GetModelMatrix() {
    return transform.to_mat4();
}

void Toilet::CleanUp() {
}