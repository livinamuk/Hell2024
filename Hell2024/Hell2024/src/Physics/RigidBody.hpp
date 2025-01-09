#pragma once
#include "Physics.h"
#include "HellCommon.h"
#include "../Util.hpp"
#include "../Renderer/Types/Model.hpp"

struct RigidBody {

    PxRigidBody* pxRigidBody = NULL;
    bool kinematic = false;

    bool Exists() {
        return pxRigidBody != NULL;
    }

    std::vector<PxShape*>& GetCollisionShapes() {
        return collisionShapes;
    }

    void SetGlobalPose(glm::mat4 matrix) {
        PxMat44 physXGlobalPose = Util::GlmMat4ToPxMat44(matrix);
        pxRigidBody->setGlobalPose(PxTransform(physXGlobalPose));
    }

    glm::mat4 GetGlobalPoseAsMatrix() {
        return Util::PxMat44ToGlmMat4(pxRigidBody->getGlobalPose());
    }

    PxTransform GetGlobalPoseAsPxTransform() {
        return pxRigidBody->getGlobalPose();
    }

    void PutToSleep() {
        if (!pxRigidBody) {
            std::cout << "You tried to put a rigid body to sleep but its pxRigidBody doesn't exist!\n";
            return;
        }
        if (!kinematic) {
            ((PxRigidDynamic*)pxRigidBody)->putToSleep();
        }
    }

    void SetKinematic(bool value) {
        if (!pxRigidBody) {
            std::cout << "You tried to set the kinematic state of a pxRigidBody that doesn't exist!\n";
            return;
        }
        kinematic = value;
        pxRigidBody->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, kinematic);
    }

    void CreateRigidBody(glm::mat4 worldMatrix) {
        Physics::Destroy(pxRigidBody);
        pxRigidBody = Physics::GetPhysics()->createRigidDynamic(PxTransform(Util::GlmMat4ToPxMat44(worldMatrix)));
        Physics::GetScene()->addActor(*pxRigidBody);
    }

    void AddCollisionShape(PxShape* shape, PhysicsFilterData physicsFilterData) {
        if (!pxRigidBody) {
            std::cout << "Tried to add a collision shape to rigid body but pxRigidBody doesn't exist!\n";
            return;
        }
        PxFilterData filterData;
        filterData.word0 = (PxU32)physicsFilterData.raycastGroup;
        filterData.word1 = (PxU32)physicsFilterData.collisionGroup;
        filterData.word2 = (PxU32)physicsFilterData.collidesWith;
        shape->setQueryFilterData(filterData);       // ray casts+
        shape->setSimulationFilterData(filterData);  // collisions
        shape = shape;
        collisionShapes.push_back(shape);
        pxRigidBody->attachShape(*shape);
    }

    void AddCollisionShapeFromModelIndex(int modelIndex, PxFilterData filterData, glm::vec3 scale) {
        if (!pxRigidBody) {
            std::cout << "Tried to add a collision shape to rigid body but pxRigidBody doesn't exist!\n";
            return;
        }
        PxConvexMesh* convexMesh = Physics::CreateConvexMeshFromModelIndex(modelIndex);
        PxShape* shape = Physics::CreateShapeFromConvexMesh(convexMesh, NULL, scale);
        shape->setQueryFilterData(filterData);       // ray casts
        shape->setSimulationFilterData(filterData);  // collisions
        collisionShapes.push_back(shape);
        pxRigidBody->attachShape(*shape);
    }

    void AddCollisionShapeFromBoundingBox(BoundingBox& boundingBox, PxFilterData filterData) {
        if (!pxRigidBody) {
            std::cout << "Tried to add a collision shape to rigid body but pxRigidBody doesn't exist!\n";
            return;
        }
        PxShape* shape = Physics::CreateBoxShape(boundingBox.size.x * 0.5f, boundingBox.size.y * 0.5f, boundingBox.size.z * 0.5f);
        shape->setQueryFilterData(filterData);       // ray casts
        shape->setSimulationFilterData(filterData);  // collisions
        collisionShapes.push_back(shape);
        pxRigidBody->attachShape(*shape);
        Transform shapeOffset;
        shapeOffset.position = boundingBox.offsetFromModelOrigin + (boundingBox.size * glm::vec3(0.5f));
        PxMat44 localShapeMatrix = Util::GlmMat4ToPxMat44(shapeOffset.to_mat4());
        PxTransform localShapeTransform(localShapeMatrix);
        shape->setLocalPose(localShapeTransform);
    }

    bool IsInMotion() {
        if (pxRigidBody) {
            PxVec3 linearVelocity = pxRigidBody->getLinearVelocity();
            PxVec3 angularVelocity = pxRigidBody->getLinearVelocity();
            return (linearVelocity.x != 0 || linearVelocity.y != 0 || linearVelocity.z != 0 || angularVelocity.x != 0 || angularVelocity.y != 0 || angularVelocity.z != 0);
        }
        else {
            return false;
        }
    }

    void Destroy() {
        Physics::Destroy(pxRigidBody);
        pxRigidBody = nullptr;
        for (PxShape* shape : collisionShapes) {
            if (shape) {
                Physics::Destroy(shape);
            }
        }
        collisionShapes.clear();
    }

private:
    std::vector<PxShape*> collisionShapes;
};