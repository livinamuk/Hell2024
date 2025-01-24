#include "Staircase.h"

glm::mat4 Staircase::GetModelMatrix() {
    Transform transform;
    transform.position = m_position;
    transform.rotation.y = m_rotation;
    return transform.to_mat4();
}

void Staircase::CreatePhysicsObjects() {

}

void Staircase::CleanupPhysicsObjects() {

}