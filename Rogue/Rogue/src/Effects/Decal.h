#pragma once
#include "../Common.h"
#include "../Core/Physics.h"

struct Decal {

    enum class Type {REGULAR = 0, GLASS};

private:
    glm::mat4 modelMatrix;
public:
    glm::vec3 localPosition;
    glm::vec3 localNormal;
    float randomRotation;
    PxRigidBody* parent;
    Type type;

    Decal();
    Decal(glm::vec3 localPosition, glm::vec3 localNormal, PxRigidBody* parent, Type type);
    void CleanUp();
    glm::mat4 GetModelMatrix();
    glm::vec3 GetWorldNormal();
};