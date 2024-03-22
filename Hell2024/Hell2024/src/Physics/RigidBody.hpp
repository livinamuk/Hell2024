#pragma once
#include "Physics.h"
#include "../Common.h"
#include "../Util.hpp"
#include "../API/OpenGL/Types/GL_mesh.h"
#include "../API/OpenGL/Types/GL_model.h"

struct RigidBody {

    PxRigidBody* pxRigidBody = NULL;
    bool kinematic = false;

    bool Exists() {
        return pxRigidBody != NULL;
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
        if (pxRigidBody) {
            pxRigidBody->release();
        }
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

    void AddCollisionShapeFromConvexMesh(OpenGLMesh* mesh, PhysicsFilterData physicsFilterData, glm::vec3 scale) {
        if (!pxRigidBody) {
            std::cout << "Tried to add a collision shape to rigid body but pxRigidBody doesn't exist!\n";
            return;
        }
        if (!mesh) {
            std::cout << "You tried to add a collision shape from an invalid mesh!\n";
            return;
        }
        if (!mesh->_convexMesh) {
            mesh->CreateConvexMesh();
        }
        PxShape* shape = Physics::CreateShapeFromConvexMesh(mesh->_convexMesh, NULL, scale);
        PxFilterData filterData;
        filterData.word0 = (PxU32)physicsFilterData.raycastGroup;
        filterData.word1 = (PxU32)physicsFilterData.collisionGroup;
        filterData.word2 = (PxU32)physicsFilterData.collidesWith;
        shape->setQueryFilterData(filterData);       // ray casts
        shape->setSimulationFilterData(filterData);  // collisions
        collisionShapes.push_back(shape);
        pxRigidBody->attachShape(*shape);
    }

    void AddCollisionShapeFromBoundingBox(BoundingBox& boundingBox, PhysicsFilterData physicsFilterData) {
        if (!pxRigidBody) {
            std::cout << "Tried to add a collision shape to rigid body but pxRigidBody doesn't exist!\n";
            return;
        }
        PxShape* shape = Physics::CreateBoxShape(boundingBox.size.x * 0.5f, boundingBox.size.y * 0.5f, boundingBox.size.z * 0.5f);
        PxFilterData filterData;
        filterData.word0 = (PxU32)physicsFilterData.raycastGroup;
        filterData.word1 = (PxU32)physicsFilterData.collisionGroup;
        filterData.word2 = (PxU32)physicsFilterData.collidesWith;
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

    void CleanUp() {
        if (pxRigidBody) {
            pxRigidBody->release();
        }
        for (auto* shape : collisionShapes) {
            if (shape) {
                if (shape) {
                    shape->release();
                }
            }
        }
    }

private:
    std::vector<OpenGLModel*> collisionModels;
    std::vector<PxShape*> collisionShapes;
};