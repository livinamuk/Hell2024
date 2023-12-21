#pragma once
#include "../Common.h"
#include "../Core/Physics.h"

struct Decal {

private:
    glm::mat4 modelMatrix;
public:
    glm::vec3 localPosition;
    glm::vec3 localNormal;
    float randomRotation;
    PxRigidBody* parent;

    Decal();
    Decal(glm::vec3 localPosition, glm::vec3 localNormal, PxRigidBody* parent);
    void CleanUp();
    glm::mat4 GetModelMatrix();
    glm::vec3 GetWorldNormal();
};