#pragma once
#include "Physics.h"
#include "HellCommon.h"
#include "../Util.hpp"

struct RigidStatic {
    PxRigidStatic* pxRigidStatic = NULL;
    PxShape* pxShape = NULL;

    void SetShape(PxShape* shape, void* parent) {
        Destroy();
        pxShape = shape;
        PhysicsFilterData filterData;
        filterData.raycastGroup = RAYCAST_ENABLED;
        filterData.collisionGroup = CollisionGroup::NO_COLLISION;
        filterData.collidesWith = CollisionGroup::NO_COLLISION;
        PxShapeFlags shapeFlags(PxShapeFlag::eSCENE_QUERY_SHAPE);
        pxRigidStatic = Physics::CreateRigidStatic(Transform(), filterData, pxShape);
        pxRigidStatic->userData = new PhysicsObjectData(ObjectType::GAME_OBJECT, parent);
    }

    void Destroy() {
        Physics::Destroy(pxRigidStatic);
        Physics::Destroy(pxShape);
    }
};
