#pragma once

#include "../Common.h"

#pragma warning(push, 0)
#include "PxPhysicsAPI.h"
#pragma warning(pop)

//#pragma warning( disable : 6495 ) // Always initialize a member variable
using namespace physx;

struct PhysicsFilterData {
	RaycastGroup raycastGroup = RaycastGroup::RAYCAST_DISABLED;
	CollisionGroup collisionGroup = CollisionGroup::NO_COLLISION;
	CollisionGroup collidesWith = CollisionGroup::ENVIROMENT_OBSTACLE;
};

struct CollisionReport {
	PxActor* rigidA = NULL;
	PxActor* rigidB = NULL;
};

struct CharacterCollisionReport {
	PxController* characterController;
	PxShape* hitShape;
	PxRigidActor* hitActor;
	glm::vec3 hitNormal;
	glm::vec3 worldPosition;
};


class CCTHitCallback : public PxUserControllerHitReport {
public:
	void onShapeHit(const PxControllerShapeHit& hit);
	void onControllerHit(const PxControllersHit& hit);
	void onObstacleHit(const PxControllerObstacleHit& hit);
};

namespace Physics {
	void Init();
	void StepPhysics(float deltaTime);
	PxTriangleMesh* CreateTriangleMesh(PxU32 numVertices, const PxVec3* vertices, PxU32 numTriangles, const PxU32* indices);
	PxConvexMesh* CreateConvexMesh(PxU32 numVertices, const PxVec3* vertices);
	PxScene* GetScene();
	PxPhysics* GetPhysics();
	PxMaterial* GetDefaultMaterial();
	PxShape* CreateBoxShape(float width, float height, float depth, Transform shapeOffset = Transform(), PxMaterial* material = NULL);
	PxRigidDynamic* CreateRigidDynamic(Transform transform, PhysicsFilterData filterData, PxShape* shape, Transform shapeOffset = Transform());
	PxRigidStatic* CreateRigidStatic(Transform transform, PhysicsFilterData physicsFilterData, PxShape* shape, Transform shapeOffset = Transform());
	PxRigidDynamic* CreateRigidDynamic(glm::mat4 matrix, PhysicsFilterData filterData, PxShape* shape);
	PxRigidDynamic* CreateRigidDynamic(glm::mat4 matrix, bool kinematic);
	PxShape* CreateShapeFromTriangleMesh(PxTriangleMesh* triangleMesh, PxShapeFlags shapeFlags, PxMaterial* material = NULL, float scale = 1);
	PxShape* CreateShapeFromConvexMesh(PxConvexMesh* convexMesh, PxMaterial* material = NULL, float scale = 1);
	void EnableRigidBodyDebugLines(PxRigidBody* rigidBody);
	void DisableRigidBodyDebugLines(PxRigidBody* rigidBody);
	std::vector<CollisionReport>& GetCollisions();
	void ClearCollisionLists();

	inline std::vector<CollisionReport> _collisionReports;
	inline std::vector<CharacterCollisionReport> _characterCollisionReports;
	inline PxControllerManager* _characterControllerManager;
	inline CCTHitCallback _cctHitCallback;

	PxRigidActor* GetGroundPlane();
}

class ContactReportCallback : public PxSimulationEventCallback {
public:
	void onConstraintBreak(PxConstraintInfo* constraints, PxU32 count) { PX_UNUSED(constraints); PX_UNUSED(count); }
	void onWake(PxActor** actors, PxU32 count) { PX_UNUSED(actors); PX_UNUSED(count); }
	void onSleep(PxActor** actors, PxU32 count) { PX_UNUSED(actors); PX_UNUSED(count); }
	void onTrigger(PxTriggerPair* pairs, PxU32 count) { PX_UNUSED(pairs); PX_UNUSED(count); }
	void onAdvance(const PxRigidBody* const*, const PxTransform*, const PxU32) {}
	void onContact(const PxContactPairHeader& pairHeader, const PxContactPair* /*pairs*/, PxU32 /*nbPairs*/) {

		/*PX_UNUSED((pairHeader));
		std::vector<PxContactPairPoint> contactPoints;

		for (PxU32 i = 0; i < nbPairs; i++) {
			PxU32 contactCount = pairs[i].contactCount;
			if (contactCount) {
				contactPoints.resize(contactCount);
				pairs[i].extractContacts(&contactPoints[0], contactCount);

				for (PxU32 j = 0; j < contactCount; j++) {

					contactPoints[j].internalFaceIndex0;

					if (!pairHeader.actors[0] || !pairHeader.actors[1]) {
						continue;
					}

					gContactPositions.push_back(contactPoints[j].position);
					gContactImpulses.push_back(contactPoints[j].impulse);
				}
			}
		}*/


		if (!pairHeader.actors[0] || !pairHeader.actors[1]) {
			return;
		}
		CollisionReport report;
		report.rigidA = (PxActor*)pairHeader.actors[0];
		report.rigidB = (PxActor*)pairHeader.actors[1];
		//	report.parentA = (PxRigidActor*)report.rigidA->userData;
		//	report.parentB = (PxRigidActor*)report.rigidB->userData;
			/*report.rigidA = (PxRigidActor*)pairHeader.actors[0];
			report.rigidB = (PxRigidActor*)pairHeader.actors[1];
			report.rigidA->getShapes(&report.shapeA, 1);
			report.rigidB->getShapes(&report.shapeB, 1);
			report.groupA = (CollisionGroup)report.shapeA->getQueryFilterData().word1;
			report.groupB = (CollisionGroup)report.shapeB->getQueryFilterData().word1;
			report.parentA = report.rigidA->userData;
			report.parentB = report.rigidB->userData;*/
		Physics::_collisionReports.push_back(report);
	}
};
