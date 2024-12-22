#pragma once
#include "HellCommon.h"

#pragma warning(push, 0)
#include "PxPhysicsAPI.h"
#include "geometry/PxGeometryHelpers.h"

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

struct OverlapReport {
	std::vector<OverlapResult> hits;
	bool HitsFound() {
		return hits.size();
	}
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
	PxScene* GetEditorScene();
    PxShape* CreateSphereShape(float radius, Transform shapeOffset = Transform(), PxMaterial* material = NULL);
	PxShape* CreateBoxShape(float width, float height, float depth, Transform shapeOffset = Transform(), PxMaterial* material = NULL);
	PxRigidDynamic* CreateRigidDynamic(Transform transform, PhysicsFilterData filterData, PxShape* shape, Transform shapeOffset = Transform());
	PxRigidStatic* CreateRigidStatic(Transform transform, PhysicsFilterData physicsFilterData, PxShape* shape, Transform shapeOffset = Transform());
	PxRigidStatic* CreateEditorRigidStatic(Transform transform, PxShape* shap, PxScene* scene);
	PxRigidDynamic* CreateRigidDynamic(glm::mat4 matrix, PhysicsFilterData filterData, PxShape* shape);
	PxRigidDynamic* CreateRigidDynamic(glm::mat4 matrix, bool kinematic);
	PxShape* CreateShapeFromTriangleMesh(PxTriangleMesh* triangleMesh, PxShapeFlags shapeFlags, PxMaterial* material = NULL, glm::vec3 scale = glm::vec3(1));
	PxShape* CreateShapeFromConvexMesh(PxConvexMesh* convexMesh, PxMaterial* material = NULL, glm::vec3 scale = glm::vec3(1));
    PxShape* CreateShapeFromHeightField(PxHeightField* heightField, PxShapeFlags shapeFlags, float heightScale, float rowScale, float colScale, PxMaterial* material = NULL);
    PxHeightField* CreateHeightField(const std::vector<Vertex>& positions, int numRows, int numCols);
    void EnableRigidBodyDebugLines(PxRigidBody* rigidBody);
	void DisableRigidBodyDebugLines(PxRigidBody* rigidBody);
	std::vector<CollisionReport>& GetCollisions();
	void ClearCollisionLists();
	OverlapReport OverlapTest(const PxGeometry& overlapShape, const PxTransform& shapePose, PxU32 collisionGroup);

    void Destroy(PxRigidDynamic*& rigidDynamic);
    void Destroy(PxRigidStatic*& rigidStatic);
    void Destroy(PxShape*& shape);
    void Destroy(PxRigidBody*& rigidBody);
    void Destroy(PxTriangleMesh*& triangleMesh);

	inline std::vector<CollisionReport> _collisionReports;
	inline std::vector<CharacterCollisionReport> _characterCollisionReports;
	inline PxControllerManager* _characterControllerManager;
	inline CCTHitCallback _cctHitCallback;

	PxRigidActor* GetGroundPlane();

    PxConvexMesh* CreateConvexMeshFromModelIndex(int modelIndex);
    PxTriangleMesh* CreateTriangleMeshFromModelIndex(int modelIndex);

    std::vector<Vertex> GetDebugLineVertices(DebugLineRenderMode debugLineRenderMode, std::vector<PxRigidActor*> ignoreList);

    void EnableRaycast(PxShape* shape);
    void DisableRaycast(PxShape* shape);

}

class ContactReportCallback : public PxSimulationEventCallback {
public:
	void onConstraintBreak(PxConstraintInfo* constraints, PxU32 count) { PX_UNUSED(constraints); PX_UNUSED(count); }
	void onWake(PxActor** actors, PxU32 count) { PX_UNUSED(actors); PX_UNUSED(count); }
	void onSleep(PxActor** actors, PxU32 count) { PX_UNUSED(actors); PX_UNUSED(count); }
	void onTrigger(PxTriggerPair* pairs, PxU32 count) { PX_UNUSED(pairs); PX_UNUSED(count); }
	void onAdvance(const PxRigidBody* const*, const PxTransform*, const PxU32) {}
	void onContact(const PxContactPairHeader& pairHeader, const PxContactPair* /*pairs*/, PxU32 /*nbPairs*/) {

		if (!pairHeader.actors[0] || !pairHeader.actors[1]) {
			return;
		}

		CollisionReport report;
		report.rigidA = pairHeader.actors[0];
		report.rigidB = pairHeader.actors[1];

		Physics::_collisionReports.push_back(report);
	}
};
