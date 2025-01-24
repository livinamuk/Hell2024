#include "Physics.h"
#include <vector>
#include <unordered_map>
#include "iostream"
#include "../Core/AssetManager.h"
#include "HellCommon.h"
#include "../Util.hpp"

/*
#ifdef _DEBUG
#define DEBUG
#else
#define NDEBUG
#endif
*/

#define PVD_HOST "127.0.0.1"

class UserErrorCallback : public PxErrorCallback
{
public:
    virtual void reportError(PxErrorCode::Enum /*code*/, const char* message, const char* file, int line) {
        std::cout << file << " line " << line << ": " << message << "\n";
        std::cout << "\n";
    }
}gErrorCallback;

PxFoundation* _foundation;
PxDefaultAllocator      _allocator;
PxPvd* _pvd = NULL;
PxPhysics* g_physics = NULL;
PxDefaultCpuDispatcher* _dispatcher = NULL;
PxScene* g_scene = NULL;
PxScene* _editorScene = NULL;
PxMaterial* _defaultMaterial = NULL;
ContactReportCallback   _contactReportCallback;
PxRigidStatic* _groundPlane = NULL;
PxShape* _groundShape = NULL;

std::unordered_map<int, PxConvexMesh*> _convexMeshes;
std::unordered_map<int, PxTriangleMesh*> _triangleMeshes;

void EnableRayCastingForShape(PxShape* shape) {
    PxFilterData filterData = shape->getQueryFilterData();
    filterData.word0 = RaycastGroup::RAYCAST_ENABLED;
    shape->setQueryFilterData(filterData);
}

void DisableRayCastingForShape(PxShape* shape) {
    PxFilterData filterData = shape->getQueryFilterData();
    filterData.word0 = RaycastGroup::RAYCAST_DISABLED;
    shape->setQueryFilterData(filterData);
}

void Physics::EnableRigidBodyDebugLines(PxRigidBody* rigidBody) {
    rigidBody->setActorFlag(PxActorFlag::eVISUALIZATION, true);
}
void Physics::DisableRigidBodyDebugLines(PxRigidBody* rigidBody) {
    rigidBody->setActorFlag(PxActorFlag::eVISUALIZATION, false);
}

void CCTHitCallback::onShapeHit(const PxControllerShapeHit& hit) {
    CharacterCollisionReport report;
	report.hitNormal = Util::PxVec3toGlmVec3(hit.worldNormal);
	report.worldPosition = Util::PxVec3toGlmVec3(hit.worldPos);
	report.characterController = hit.controller;
	report.hitShape = hit.shape;
	report.hitActor = hit.actor;
    Physics::_characterCollisionReports.push_back(report);
}

void CCTHitCallback::onControllerHit(const PxControllersHit& hit){
}

void CCTHitCallback::onObstacleHit(const PxControllerObstacleHit& hit) {
}

PxFilterFlags contactReportFilterShader(PxFilterObjectAttributes attributes0, PxFilterData filterData0, PxFilterObjectAttributes attributes1, PxFilterData filterData1, PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize) {

    PX_UNUSED(attributes0);
    PX_UNUSED(attributes1);
    PX_UNUSED(constantBlockSize);
    PX_UNUSED(constantBlock);

    // let triggers through
    if (PxFilterObjectIsTrigger(attributes0) || PxFilterObjectIsTrigger(attributes1)) {
        //   pairFlags = PxPairFlag::eTRIGGER_DEFAULT;
       //    return PxFilterFlag::eDEFAULT;
    }

    // generate contacts for all that were not filtered above
    pairFlags = PxPairFlag::eCONTACT_DEFAULT;

    if (filterData0.word2 == CollisionGroup::NO_COLLISION)
        return PxFilterFlag::eKILL;
    //else if (filterData0.word2 == CollisionGroup::NO_COLLISION)
    //    return PxFilterFlag::eKILL;

    // trigger the contact callback for pairs (A,B) where
    // the filtermask of A contains the ID of B and vice versa.
    else if ((filterData0.word2 & filterData1.word1) && (filterData1.word2 & filterData0.word1)) {

        //if (filterData0.word2 & BULLET_CASING && filterData1.word1 & ENVIROMENT_OBSTACLE ||
        //    filterData0.word1 & BULLET_CASING && filterData1.word2 & ENVIROMENT_OBSTACLE) {
        //    Physics::_shellCollisionsThisFrame++;
        //}

        pairFlags |= PxPairFlag::eNOTIFY_TOUCH_FOUND;
        return PxFilterFlag::eDEFAULT;
    }

    return PxFilterFlag::eKILL;
}

/*
PxFilterFlags PhysicsMainFilterShader(PxFilterObjectAttributes attributes0, PxFilterData filterData0, PxFilterObjectAttributes attributes1, PxFilterData filterData1, PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize) {
    // generate contacts for all that were not filtered above
    pairFlags = PxPairFlag::eCONTACT_DEFAULT | PxPairFlag::eTRIGGER_DEFAULT | PxPairFlag::eNOTIFY_CONTACT_POINTS;
    return PxFilterFlag::eDEFAULT;
}*/


// Setup common cooking params
void SetupCommonCookingParams(PxCookingParams& params, bool skipMeshCleanup, bool skipEdgeData) {
    // we suppress the triangle mesh remap table computation to gain some speed, as we will not need it
// in this snippet
    params.suppressTriangleMeshRemapTable = true;

    // If DISABLE_CLEAN_MESH is set, the mesh is not cleaned during the cooking. The input mesh must be valid.
    // The following conditions are true for a valid triangle mesh :
    //  1. There are no duplicate vertices(within specified vertexWeldTolerance.See PxCookingParams::meshWeldTolerance)
    //  2. There are no large triangles(within specified PxTolerancesScale.)
    // It is recommended to run a separate validation check in debug/checked builds, see below.

    if (!skipMeshCleanup)
        params.meshPreprocessParams &= ~static_cast<PxMeshPreprocessingFlags>(PxMeshPreprocessingFlag::eDISABLE_CLEAN_MESH);
    else
        params.meshPreprocessParams |= PxMeshPreprocessingFlag::eDISABLE_CLEAN_MESH;

    // If eDISABLE_ACTIVE_EDGES_PRECOMPUTE is set, the cooking does not compute the active (convex) edges, and instead
    // marks all edges as active. This makes cooking faster but can slow down contact generation. This flag may change
    // the collision behavior, as all edges of the triangle mesh will now be considered active.
    if (!skipEdgeData)
        params.meshPreprocessParams &= ~static_cast<PxMeshPreprocessingFlags>(PxMeshPreprocessingFlag::eDISABLE_ACTIVE_EDGES_PRECOMPUTE);
    else
        params.meshPreprocessParams |= PxMeshPreprocessingFlag::eDISABLE_ACTIVE_EDGES_PRECOMPUTE;
}

PxTriangleMesh* Physics::CreateTriangleMesh(PxU32 numVertices, const PxVec3* vertices, PxU32 numTriangles, const PxU32* indices) {

    PxTriangleMeshDesc meshDesc;
    meshDesc.points.count = numVertices;
    meshDesc.points.data = vertices;
    meshDesc.points.stride = sizeof(PxVec3);
    meshDesc.triangles.count = numTriangles;
    meshDesc.triangles.data = indices;
    meshDesc.triangles.stride = 3 * sizeof(PxU32);

    PxTolerancesScale scale;
    PxCookingParams params(scale);

    // Create BVH33 midphase
    params.midphaseDesc = PxMeshMidPhase::eBVH33;

    // setup common cooking params
    bool skipMeshCleanup = false;
    bool skipEdgeData = false;
    bool cookingPerformance = false;
    bool meshSizePerfTradeoff = true;
    SetupCommonCookingParams(params, skipMeshCleanup, skipEdgeData);

    // The COOKING_PERFORMANCE flag for BVH33 midphase enables a fast cooking path at the expense of somewhat lower quality BVH construction.
    if (cookingPerformance) {
        params.midphaseDesc.mBVH33Desc.meshCookingHint = PxMeshCookingHint::eCOOKING_PERFORMANCE;
    }
    else {
        params.midphaseDesc.mBVH33Desc.meshCookingHint = PxMeshCookingHint::eSIM_PERFORMANCE;
    }

    // If meshSizePerfTradeoff is set to true, smaller mesh cooked mesh is produced. The mesh size/performance trade-off
    // is controlled by setting the meshSizePerformanceTradeOff from 0.0f (smaller mesh) to 1.0f (larger mesh).
    if (meshSizePerfTradeoff) {
        params.midphaseDesc.mBVH33Desc.meshSizePerformanceTradeOff = 0.0f;
    }
    else {
        // using the default value
        params.midphaseDesc.mBVH33Desc.meshSizePerformanceTradeOff = 0.55f;
    }
    if (skipMeshCleanup) {
        PX_ASSERT(PxValidateTriangleMesh(params, meshDesc));
    }

    PxTriangleMesh* triMesh = NULL;
    //PxU32 meshSize = 0;

    triMesh = PxCreateTriangleMesh(params, meshDesc, g_physics->getPhysicsInsertionCallback());
    return triMesh;
    //triMesh->release();
}



PxConvexMesh* Physics::CreateConvexMesh(PxU32 numVertices, const PxVec3* vertices) {
    PxConvexMeshDesc convexDesc;
    convexDesc.points.count = numVertices;
    convexDesc.points.stride = sizeof(PxVec3);
    convexDesc.points.data = vertices;
    convexDesc.flags = PxConvexFlag::eSHIFT_VERTICES | PxConvexFlag::eCOMPUTE_CONVEX;
    //  s
    PxTolerancesScale scale;
    PxCookingParams params(scale);

    PxDefaultMemoryOutputStream buf;
    PxConvexMeshCookingResult::Enum result;
    if (!PxCookConvexMesh(params, convexDesc, buf, &result)) {
        return NULL;
    }
    PxDefaultMemoryInputData input(buf.getData(), buf.getSize());
    return g_physics->createConvexMesh(input);
}



void Physics::Init() {

    _foundation = PxCreateFoundation(PX_PHYSICS_VERSION, _allocator, gErrorCallback);
    if (!_foundation) {
        std::cout << "PxCreateFoundation failed!\n";
    }
    else {
        //std::cout << "PxCreateFoundation create successfully!\n";
    }

    _pvd = PxCreatePvd(*_foundation);
    PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate(PVD_HOST, 5425, 10);
    _pvd->connect(*transport, PxPvdInstrumentationFlag::eALL);

    g_physics = PxCreatePhysics(PX_PHYSICS_VERSION, *_foundation, PxTolerancesScale(), true, _pvd);
    if (!g_physics) {
        std::cout << "PxCreatePhysics failed!\n";
    }

    PxSceneDesc sceneDesc(g_physics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
    _dispatcher = PxDefaultCpuDispatcherCreate(2);
    sceneDesc.cpuDispatcher = _dispatcher;
    //sceneDesc.filterShader = PxDefaultSimulationFilterShader;
   // sceneDesc.filterShader = FilterShaderExample;
    sceneDesc.filterShader = contactReportFilterShader;
    sceneDesc.simulationEventCallback = &_contactReportCallback;

    g_scene = g_physics->createScene(sceneDesc);
    g_scene->setVisualizationParameter(PxVisualizationParameter::eSCALE, 1.0f);
    g_scene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_SHAPES, 2.0f);

	_editorScene = g_physics->createScene(sceneDesc);
	_editorScene->setVisualizationParameter(PxVisualizationParameter::eSCALE, 1.0f);
	_editorScene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_SHAPES, 2.0f);

    PxPvdSceneClient* pvdClient = g_scene->getScenePvdClient();
    if (pvdClient) {
        pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
        pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
        pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
    }
    _defaultMaterial = g_physics->createMaterial(0.5f, 0.5f, 0.6f);

    // Character controller shit
    _characterControllerManager = PxCreateControllerManager(*g_scene);



    if (false) {
        _groundPlane = PxCreatePlane(*g_physics, PxPlane(0, 1, 0, 0.0f), *_defaultMaterial);
        g_scene->addActor(*_groundPlane);
        _groundPlane->getShapes(&_groundShape, 1);
        PxFilterData filterData;
        filterData.word0 = RaycastGroup::RAYCAST_DISABLED; // must be disabled or it causes crash in scene::update when it tries to retrieve rigid body flags from this actor
        filterData.word1 = CollisionGroup::ENVIROMENT_OBSTACLE;
        filterData.word2 = CollisionGroup::BULLET_CASING | CollisionGroup::GENERIC_BOUNCEABLE | CollisionGroup::PLAYER;
        _groundShape->setQueryFilterData(filterData);
        _groundShape->setSimulationFilterData(filterData); // sim is for ragz
    }
}

void Physics::StepPhysics(float deltaTime) {
    g_scene->simulate(deltaTime);
    g_scene->fetchResults(true);
}

PxScene* Physics::GetScene() {
    return g_scene;
}
PxScene* Physics::GetEditorScene() {
	return _editorScene;
}

PxPhysics* Physics::GetPhysics() {
    return g_physics;
}

PxMaterial* Physics::GetDefaultMaterial() {
    return _defaultMaterial;
}

PxShape* Physics::CreateSphereShape(float radius, Transform shapeOffset, PxMaterial* material) {
    if (material == NULL) {
        material = _defaultMaterial;
    }
    PxShape* shape = g_physics->createShape(PxSphereGeometry(radius), *material, true);
    PxMat44 localShapeMatrix = Util::GlmMat4ToPxMat44(shapeOffset.to_mat4());
    PxTransform localShapeTransform(localShapeMatrix);
    shape->setLocalPose(localShapeTransform);
    return shape;
}

PxShape* Physics::CreateBoxShape(float width, float height, float depth, Transform shapeOffset, PxMaterial* material) {
    if (material == NULL) {
        material = _defaultMaterial;
    }
    PxShape* shape = g_physics->createShape(PxBoxGeometry(width, height, depth), *material, true);
    PxMat44 localShapeMatrix = Util::GlmMat4ToPxMat44(shapeOffset.to_mat4());
    PxTransform localShapeTransform(localShapeMatrix);
    shape->setLocalPose(localShapeTransform);
    return shape;
}

PxShape* Physics::CreateShapeFromTriangleMesh(PxTriangleMesh* triangleMesh, PxShapeFlags shapeFlags2, PxMaterial* material, glm::vec3 scale) {
    if (material == NULL) {
        material = _defaultMaterial;
	}
	//PxMeshGeometryFlags flags(~PxMeshGeometryFlag::eTIGHT_BOUNDS | ~PxMeshGeometryFlag::eDOUBLE_SIDED);
    // maybe tight bounds did something?? check it out...
	PxMeshGeometryFlags flags(~PxMeshGeometryFlag::eDOUBLE_SIDED);
    PxTriangleMeshGeometry geometry(triangleMesh, PxMeshScale(PxVec3(scale.x, scale.y, scale.z)), flags);

    PxShapeFlags shapeFlags(PxShapeFlag::eSCENE_QUERY_SHAPE); // Most importantly NOT eSIMULATION_SHAPE. PhysX does not allow for tri mesh.
    return g_physics->createShape(geometry, *material, shapeFlags);
}


PxShape* Physics::CreateShapeFromConvexMesh(PxConvexMesh* convexMesh, PxMaterial* material, glm::vec3 scale) {
    if (material == NULL) {
        material = _defaultMaterial;
    }
    PxConvexMeshGeometryFlags flags(~PxConvexMeshGeometryFlag::eTIGHT_BOUNDS);
    PxConvexMeshGeometry geometry(convexMesh, PxMeshScale(PxVec3(scale.x, scale.y, scale.z)), flags);
    return g_physics->createShape(geometry, *material);
}


PxRigidDynamic* Physics::CreateRigidDynamic(Transform transform, PhysicsFilterData physicsFilterData, PxShape* shape, Transform shapeOffset) {

    PxQuat quat = Util::GlmQuatToPxQuat(glm::quat(transform.rotation));
    PxTransform trans = PxTransform(PxVec3(transform.position.x, transform.position.y, transform.position.z), quat);
    PxRigidDynamic* body = g_physics->createRigidDynamic(trans);

    // You are passing in a PxShape pointer and any shape offset will affects that actually object, wherever the fuck it is up the function chain.
    // Maybe look into this when you can be fucked, possibly you can just set the isExclusive bool to true, where and whenever the fuck that is and happens.
    PxFilterData filterData;
    filterData.word0 = (PxU32)physicsFilterData.raycastGroup;
    filterData.word1 = (PxU32)physicsFilterData.collisionGroup;
    filterData.word2 = (PxU32)physicsFilterData.collidesWith;
    shape->setQueryFilterData(filterData);       // ray casts
    shape->setSimulationFilterData(filterData);  // collisions
    PxMat44 localShapeMatrix = Util::GlmMat4ToPxMat44(shapeOffset.to_mat4());
    PxTransform localShapeTransform(localShapeMatrix);
    shape->setLocalPose(localShapeTransform);

    body->attachShape(*shape);
    PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);
    g_scene->addActor(*body);
    return body;
}


PxRigidStatic* Physics::CreateRigidStatic(Transform transform, PhysicsFilterData physicsFilterData, PxShape* shape, Transform shapeOffset) {

    PxQuat quat = Util::GlmQuatToPxQuat(glm::quat(transform.rotation));
    PxTransform trans = PxTransform(PxVec3(transform.position.x, transform.position.y, transform.position.z), quat);
    PxRigidStatic* body = g_physics->createRigidStatic(trans);

    // You are passing in a PxShape pointer and any shape offset will affects that actually object, wherever the fuck it is up the function chain.
    // Maybe look into this when you can be fucked, possibly you can just set the isExclusive bool to true, where and whenever the fuck that is and happens.
    PxFilterData filterData;
    filterData.word0 = (PxU32)physicsFilterData.raycastGroup;
    filterData.word1 = (PxU32)physicsFilterData.collisionGroup;
    filterData.word2 = (PxU32)physicsFilterData.collidesWith;
    shape->setQueryFilterData(filterData);       // ray casts
    shape->setSimulationFilterData(filterData);  // collisions
    PxMat44 localShapeMatrix = Util::GlmMat4ToPxMat44(shapeOffset.to_mat4());
    PxTransform localShapeTransform(localShapeMatrix);
    shape->setLocalPose(localShapeTransform);

    body->attachShape(*shape);
    g_scene->addActor(*body);

    return body;
}

PxRigidStatic* Physics::CreateEditorRigidStatic(Transform transform, PxShape* shape, PxScene* scene) {
    PxQuat quat = Util::GlmQuatToPxQuat(glm::quat(transform.rotation));
	PxTransform trans = PxTransform(PxVec3(transform.position.x, transform.position.y, transform.position.z), quat);

	PxRigidStatic* body = g_physics->createRigidStatic(trans);
	//PxRigidStatic* body = _physics->createRigidStatic(PxTransform());
	body->attachShape(*shape);
    PxFilterData filterData;
    filterData.word0 = (PxU32)RAYCAST_ENABLED;
    filterData.word1 = (PxU32)NO_COLLISION;
    filterData.word2 = (PxU32)NO_COLLISION;
	shape->setQueryFilterData(filterData);       // ray casts
	//body->setActorFlag(PxActorFlag::eVISUALIZATION, true);
    scene->addActor(*body);
    return body;
}



PxRigidDynamic* Physics::CreateRigidDynamic(glm::mat4 matrix, PhysicsFilterData physicsFilterData, PxShape* shape) {
    PxFilterData filterData;
    filterData.word0 = (PxU32)physicsFilterData.raycastGroup;
    filterData.word1 = (PxU32)physicsFilterData.collisionGroup;
    filterData.word2 = (PxU32)physicsFilterData.collidesWith;
    shape->setQueryFilterData(filterData);       // ray casts
    shape->setSimulationFilterData(filterData);  // collisions
    PxMat44 mat = Util::GlmMat4ToPxMat44(matrix);
    PxTransform transform(mat);
    PxRigidDynamic* body = g_physics->createRigidDynamic(transform);
    body->attachShape(*shape);
    PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);
    g_scene->addActor(*body);
    return body;
}

PxRigidDynamic* Physics::CreateRigidDynamic(glm::mat4 matrix, bool kinematic) {
    PxMat44 mat = Util::GlmMat4ToPxMat44(matrix);
    PxTransform transform(mat);
    PxRigidDynamic* body = g_physics->createRigidDynamic(transform);
    body->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, kinematic);
    g_scene->addActor(*body);
    return body;
}

std::vector<CollisionReport>& Physics::GetCollisions() {
    return _collisionReports;
}
void Physics::ClearCollisionLists() {
    _collisionReports.clear();
    _characterCollisionReports.clear();
}

physx::PxRigidActor* Physics::GetGroundPlane() {
    return _groundPlane;
}

OverlapReport Physics::OverlapTest(const PxGeometry& overlapShape, const PxTransform& shapePose, PxU32 collisionGroup) {
	PxQueryFilterData overlapFilterData = PxQueryFilterData();
	overlapFilterData.data.word1 = collisionGroup;
	const PxU32 bufferSize = 256;
	PxOverlapHit hitBuffer[bufferSize];
	PxOverlapBuffer buf(hitBuffer, bufferSize);
    std::vector<PxActor*> hitActors;
	if (Physics::GetScene()->overlap(overlapShape, shapePose, buf, overlapFilterData)) {
		for (int i = 0; i < buf.getNbTouches(); i++) {
			PxActor* hit = buf.getTouch(i).actor;
			// Check for duplicates
			bool found = false;
			for (const PxActor* foundHit : hitActors) {
				if (foundHit == hit) {
					found = true;
					break;
				}
			}
			if (!found) {
                hitActors.push_back(hit);
			}
		}
	}

    // Fill out the shit you need
    OverlapReport overlapReport;
    for (PxActor* hitActor : hitActors) {
        PhysicsObjectData* physicsObjectData = (PhysicsObjectData*)hitActor->userData;
        if (physicsObjectData) {
            PxRigidBody* rigid = (PxRigidBody*)hitActor;
            OverlapResult& overlapResult = overlapReport.hits.emplace_back();
            overlapResult.objectType = physicsObjectData->type;
            overlapResult.parent = physicsObjectData->parent;
            overlapResult.position.x = rigid->getGlobalPose().p.x;
            overlapResult.position.y = rigid->getGlobalPose().p.y;
            overlapResult.position.z = rigid->getGlobalPose().p.z;
        }
    }
	return overlapReport;
}

PxConvexMesh* Physics::CreateConvexMeshFromModelIndex(int modelIndex) {

    // Create convex mesh if doesn't exist yet
    if (!_convexMeshes.contains(modelIndex)) {

        // Retrieve model if it exists
        Model* model = AssetManager::GetModelByIndex(modelIndex);
        if (!model) {
            std::cout << "Physics::CreateConvexMeshFromModelIndex() failed. Index " << modelIndex << " was invalid!\n";
            return nullptr;
        }
        // Create it
        std::vector<PxVec3> vertices;
        for (unsigned int meshIndex : model->GetMeshIndices()) {
            Mesh* mesh = AssetManager::GetMeshByIndex(meshIndex);
            for (int i = mesh->baseVertex; i < mesh->baseVertex + mesh->vertexCount; i++) {
                Vertex& vertex = AssetManager::GetVertices()[i];
                vertices.push_back(PxVec3(vertex.position.x, vertex.position.y, vertex.position.z));
            }
        }
        if (vertices.size()) {
            _convexMeshes[modelIndex] = Physics::CreateConvexMesh(vertices.size(), &vertices[0]);
            return _convexMeshes[modelIndex];
        }
        std::cout << "Physics::CreateConvexMeshFromModelIndex() failed. Vertices has size 0!\n";
        return nullptr;
    }
    // Return it if it already exists
    else {
        return _convexMeshes[modelIndex];
    }

}

PxTriangleMesh* Physics::CreateTriangleMeshFromModelIndex(int modelIndex) {

    // Create convex mesh if doesn't exist yet
    if (!_triangleMeshes.contains(modelIndex)) {

        // Retrieve model if it exists
        Model* model = AssetManager::GetModelByIndex(modelIndex);
        if (!model) {
            std::cout << "Physics::CreateTriangleMeshFromModelIndex() failed. Index " << modelIndex << " was invalid!\n";
            return nullptr;
        }
        std::vector<PxVec3> pxvertices;
        std::vector<unsigned int> pxindices;
        int pxBaseVertex = 0;
        for (unsigned int meshIndex : model->GetMeshIndices()) {
            Mesh* mesh = AssetManager::GetMeshByIndex(meshIndex);

            for (int i = 0; i < mesh->vertexCount; i++) {
                Vertex& vertex = AssetManager::GetVertices()[i + mesh->baseVertex];
                pxvertices.push_back(PxVec3(vertex.position.x, vertex.position.y, vertex.position.z));

            }
            for (int i = 0; i < mesh->indexCount; i++) {
                unsigned int index = AssetManager::GetIndices()[i + mesh->baseIndex];
                pxindices.push_back(index + pxBaseVertex);
            }
            pxBaseVertex = pxvertices.size();
        }
        if (pxvertices.size()) {
            _triangleMeshes[modelIndex] = Physics::CreateTriangleMesh(pxvertices.size(), pxvertices.data(), pxindices.size() / 3, pxindices.data());
            return  _triangleMeshes[modelIndex];
        }
        else {
            std::cout << "Physics::CreateTriangleMeshFromModelIndex() failed. Vertices has size 0!\n";
            return nullptr;
        }
    }
    // Return it if it already exists
    else {
        return _triangleMeshes[modelIndex];
    }
}

std::vector<Vertex> Physics::GetDebugLineVertices(DebugLineRenderMode debugLineRenderMode, std::vector<PxRigidActor*> ignoreList) {

    // Prepare
    PxU32 nbActors = g_scene->getNbActors(PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC);
    if (nbActors) {
        std::vector<PxRigidActor*> actors(nbActors);
        g_scene->getActors(PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC, reinterpret_cast<PxActor**>(&actors[0]), nbActors);
        for (PxRigidActor* actor : actors) {
            if (actor == Physics::GetGroundPlane()) {
                actor->setActorFlag(PxActorFlag::eVISUALIZATION, false);
                continue;
            }
            PxShape* shape;
            actor->getShapes(&shape, 1);
            actor->setActorFlag(PxActorFlag::eVISUALIZATION, true);
            for (PxRigidActor* ignoredActor : ignoreList) {
                if (ignoredActor == actor) {
                    actor->setActorFlag(PxActorFlag::eVISUALIZATION, false);
                }
            }
            if (debugLineRenderMode == DebugLineRenderMode::PHYSX_RAYCAST) {
                if (shape->getQueryFilterData().word0 == RaycastGroup::RAYCAST_DISABLED) {
                    actor->setActorFlag(PxActorFlag::eVISUALIZATION, false);
                }
            }
            else if (debugLineRenderMode == DebugLineRenderMode::PHYSX_COLLISION) {
                if (shape->getQueryFilterData().word1 == CollisionGroup::NO_COLLISION) {
                    actor->setActorFlag(PxActorFlag::eVISUALIZATION, false);
                }
            }
        }
    }
    // Build vertices
    std::vector<Vertex> vertices;
    auto* renderBuffer = &g_scene->getRenderBuffer();
    for (unsigned int i = 0; i < renderBuffer->getNbLines(); i++) {
        auto pxLine = renderBuffer->getLines()[i];
        Vertex v1, v2;
        v1.position = Util::PxVec3toGlmVec3(pxLine.pos0);
        v2.position = Util::PxVec3toGlmVec3(pxLine.pos1);
        if (debugLineRenderMode == DebugLineRenderMode::PHYSX_ALL) {
            v1.normal = GREEN;
            v2.normal = GREEN;
        }
        else if (debugLineRenderMode == DebugLineRenderMode::PHYSX_COLLISION) {
            v1.normal = LIGHT_BLUE;
            v2.normal = LIGHT_BLUE;
        }
        else if (debugLineRenderMode == DebugLineRenderMode::PHYSX_RAYCAST) {
            v1.normal = RED;
            v2.normal = RED;
        }
        vertices.push_back(v1);
        vertices.push_back(v2);
    }
    return vertices;
}


void Physics::EnableRaycast(PxShape* shape) {
    PxFilterData filterData = shape->getQueryFilterData();
    filterData.word0 = RaycastGroup::RAYCAST_ENABLED;
    shape->setQueryFilterData(filterData);
}

void Physics::DisableRaycast(PxShape* shape) {
    PxFilterData filterData = shape->getQueryFilterData();
    filterData.word0 = RaycastGroup::RAYCAST_DISABLED;
    shape->setQueryFilterData(filterData);
}

PxHeightField* Physics::CreateHeightField( const std::vector<Vertex>& positions, int numRows, int numCols) {

    if (positions.size() != numRows * numCols) {
        std::cout << "Physics::CreateHeightField() failed because positions.size() != numRows * numCols\n";
        return nullptr;
    }
    const float heightScaleFactor = 32767.0f;  // Using the full PxI16 range

    std::vector<PxHeightFieldSample> samples(numRows * numCols);

    for (int z = 0; z < numCols; z++) { 
        for (int x = 0; x < numRows; x++) {
            int vertexIndex = (z * numRows + x);
            int sampleIndex = (x * numCols + z);
            samples[sampleIndex].height = static_cast<PxI16>(positions[vertexIndex].position.y * heightScaleFactor);
            samples[sampleIndex].materialIndex0 = 0;
            samples[sampleIndex].materialIndex1 = 0;
            samples[sampleIndex].setTessFlag();
        }
    }
    // Height field description
    PxHeightFieldDesc heightFieldDesc;
    heightFieldDesc.format = PxHeightFieldFormat::eS16_TM;
    heightFieldDesc.nbRows = numRows;
    heightFieldDesc.nbColumns = numCols;
    heightFieldDesc.samples.data = samples.data();
    heightFieldDesc.samples.stride = sizeof(PxHeightFieldSample);
    if (!heightFieldDesc.isValid()) {
        std::cout << "Failed to create PxHeightField\n";
        return nullptr; // Invalid height field description
    }
    PxHeightField* heightField = PxCreateHeightField(heightFieldDesc, g_physics->getPhysicsInsertionCallback());
    return heightField;
}

PxShape* Physics::CreateShapeFromHeightField(PxHeightField* heightField, PxShapeFlags shapeFlags, float heightScale, float rowScale, float colScale, PxMaterial* material) {
    if (material == NULL) {
        material = _defaultMaterial;
    }
    float worldHeightScale = 1.0f / 32767.0f * heightScale;
    PxHeightFieldGeometry hfGeom(heightField, PxMeshGeometryFlags(), worldHeightScale, rowScale, colScale);
    PxShape* shape = g_physics->createShape(hfGeom, *material, shapeFlags);
    shape->setFlag(PxShapeFlag::eVISUALIZATION, false);
    return shape;
}

void Physics::Destroy(PxRigidDynamic*& rigidDynamic) {
    if (rigidDynamic) {
        if (rigidDynamic->userData) {
            delete static_cast<PhysicsObjectData*>(rigidDynamic->userData);
            rigidDynamic->userData = nullptr;
        }
        g_scene->removeActor(*rigidDynamic);
        rigidDynamic->release();
        rigidDynamic = nullptr;
    }
}

void Physics::Destroy(PxRigidStatic*& rigidStatic) {
    if (rigidStatic) {
        if (rigidStatic->userData) {
            delete static_cast<PhysicsObjectData*>(rigidStatic->userData);
            rigidStatic->userData = nullptr;
        }
        g_scene->removeActor(*rigidStatic);
        rigidStatic->release();
        rigidStatic = nullptr;
    }
}

void Physics::Destroy(PxShape*& shape) {
    if (shape) {
        if (shape->userData) {
            delete static_cast<PhysicsObjectData*>(shape->userData);
            shape->userData = nullptr;
        }
        shape->release();
        shape = nullptr;
    }
}

void Physics::Destroy(PxRigidBody*& rigidBody) {
    if (rigidBody) {
        if (rigidBody->userData) {
            delete static_cast<PhysicsObjectData*>(rigidBody->userData);
            rigidBody->userData = nullptr;
        }
        g_scene->removeActor(*rigidBody);
        rigidBody->release();
        rigidBody = nullptr;
    }
}

void Physics::Destroy(PxTriangleMesh*& triangleMesh) {
    if (triangleMesh) {
        triangleMesh = nullptr;
    }
}
