#include "Decal.h"
#include "../Util.hpp"

Decal::Decal() {}

Decal::Decal(glm::vec3 localPosition, glm::vec3 localNormal, PxRigidBody * parent, Type type) {
    float min = 0;
    float max = HELL_PI * 2;
    float randomFloat = min + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (max - min)));
    randomRotation = randomFloat;
    this->localPosition = localPosition;
    this->localNormal = localNormal;
    this->parent = parent;
    this->type = type;
}

void Decal::CleanUp() {
    
}

glm::mat4 Decal::GetModelMatrix() {

    glm::mat4 parentMatrix = Util::PxMat44ToGlmMat4(parent->getGlobalPose());

    Transform localTranslation;
    localTranslation.position = localPosition;// +(normal * glm::vec3(0.001));
    localTranslation.scale = glm::vec3(0.02f);
    //localTranslation.scale = glm::vec3(0.2);

    Transform localRotation;
    localRotation.rotation.z = randomRotation;

    glm::vec3 n = localNormal;
    float sign = copysignf(1.0f, n.z);
    const float a = -1.0f / (sign + n.z);
    const float b = n.x * n.y * a;
    glm::vec3 b1 = glm::vec3(1.0f + sign * n.x * n.x * a, sign * b, -sign * n.x);
    glm::vec3 b2 = glm::vec3(b, sign + n.y * n.y * a, -n.y);

    glm::mat4 rotationMatrix = glm::mat4(1);
    rotationMatrix[0] = glm::vec4(b1, 0);
    rotationMatrix[1] = glm::vec4(b2, 0);
    rotationMatrix[2] = glm::vec4(n, 0);

    modelMatrix = parentMatrix * localTranslation.to_mat4() * rotationMatrix * localRotation.to_mat4();
    return modelMatrix;
}

glm::vec3 Decal::GetWorldNormal() {
    return glm::vec3(0);
}
