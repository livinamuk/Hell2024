#pragma once
#include "CSGShape.h"

glm::mat4 CSGShape::GetModelMatrix() {
    return m_transform.to_mat4();
}

glm::mat4 CSGShape::GetNormalMatrix() {
    Transform transform;
    transform.position = m_transform.position;
    transform.rotation = m_transform.rotation;
    return transform.to_mat4();
}

void CSGShape::SetTransform(Transform transform) {
    m_transform = transform;
    if (pxRigidStatic) {
        PxMat44 matrix = Util::GlmMat4ToPxMat44(GetModelMatrix());
        PxTransform transform2 = PxTransform(matrix);
        pxRigidStatic->setGlobalPose(transform2);
    }
}

Transform& CSGShape::GetTransform() {
    return m_transform;
}

void CSGShape::CleanUp() {
    Physics::Destroy(pxRigidStatic);
    Physics::Destroy(m_pxShape);
}

void CSGShape::CreateCubePhysicsObject() {

    PhysicsFilterData filterData2;
    filterData2.raycastGroup = RaycastGroup::RAYCAST_ENABLED;
    filterData2.collisionGroup = NO_COLLISION;
    filterData2.collidesWith = NO_COLLISION;
    PxShapeFlags shapeFlags(PxShapeFlag::eSCENE_QUERY_SHAPE); // Most importantly NOT eSIMULATION_SHAPE. PhysX does not allow for tri mesh.

    float width = m_transform.scale.x * 0.5f;
    float height = m_transform.scale.y * 0.5f;
    float depth = m_transform.scale.z * 0.5f;

    m_pxShape = Physics::CreateBoxShape(width, height, depth);
    pxRigidStatic = Physics::CreateRigidStatic(Transform(), filterData2, m_pxShape);

    PhysicsObjectData* physicsObjectData = new PhysicsObjectData(PhysicsObjectType::CSG_OBJECT_SUBTRACTIVE, this);
    pxRigidStatic->userData = physicsObjectData;

    PxMat44 m2 = Util::GlmMat4ToPxMat44(GetModelMatrix());
    PxTransform transform2 = PxTransform(m2);
    pxRigidStatic->setGlobalPose(transform2);
    Physics::DisableRaycast(m_pxShape);
}

void CSGShape::DisableRaycast() {
    Physics::DisableRaycast(m_pxShape);
}

void CSGShape::EnableRaycast() {
    Physics::EnableRaycast(m_pxShape);
}