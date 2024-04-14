#pragma once
#include "Physics.h"
#include "../Common.h"
#include "../Util.hpp"

struct RigidStatic {
    PxRigidStatic* pxRigidStatic = NULL;
    PxShape* pxShape = NULL;

    void SetShape(PxShape* shape, void* parent) {
        pxShape = shape;
        if (pxRigidStatic) {
            pxRigidStatic->release();
        }
        PhysicsFilterData filterData;
        filterData.raycastGroup = RAYCAST_ENABLED;
        filterData.collisionGroup = CollisionGroup::NO_COLLISION;
        filterData.collidesWith = CollisionGroup::NO_COLLISION;
        PxShapeFlags shapeFlags(PxShapeFlag::eSCENE_QUERY_SHAPE);
        pxRigidStatic = Physics::CreateRigidStatic(Transform(), filterData, pxShape);
        pxRigidStatic->userData = new PhysicsObjectData(PhysicsObjectType::GAME_OBJECT, parent);
    }

    void CleanUp() {       
        if (pxRigidStatic) {
            pxRigidStatic->release();
        }
        if (pxShape) {
            pxShape->release();
        }
    }
};
