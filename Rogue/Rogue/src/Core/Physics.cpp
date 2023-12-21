#include "Physics.h"
#include <vector>
#include "iostream"
#include "../Common.h"
#include "../Util.hpp"

#ifdef _DEBUG 
#define DEBUG
#else
#define NDEBUG
#endif
#define PVD_HOST "127.0.0.1"


class UserErrorCallback : public PxErrorCallback
{
public:
    virtual void reportError(PxErrorCode::Enum code, const char* message, const char* file, int line) {
        std::cout << file << " line " << line << ": " << message << "\n";
        std::cout << "\n";
    }
}gErrorCallback;

PxFoundation*           _foundation;
PxDefaultAllocator      _allocator;
PxPvd*                  _pvd = NULL;
PxPhysics*              _physics = NULL;
PxDefaultCpuDispatcher* _dispatcher = NULL;
PxScene*                _scene = NULL;
PxMaterial*             _defaultMaterial = NULL;
ContactReportCallback   _contactReportCallback;

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

PxFilterFlags FilterShaderExample(
    PxFilterObjectAttributes attributes0, PxFilterData filterData0,
    PxFilterObjectAttributes attributes1, PxFilterData filterData1,
    PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
{
    // let triggers through
    if (PxFilterObjectIsTrigger(attributes0) || PxFilterObjectIsTrigger(attributes1)) {
        pairFlags = PxPairFlag::eTRIGGER_DEFAULT;
        return PxFilterFlag::eDEFAULT;
    }
    // generate contacts for all that were not filtered above
    pairFlags = PxPairFlag::eCONTACT_DEFAULT;

    // trigger the contact callback for pairs (A,B) where
    // the filtermask of A contains the ID of B and vice versa.
    //if ((filterData0.word0 & filterData1.word1) && (filterData1.word0 & filterData0.word1))
    if ((filterData0.word2 & filterData1.word1) && (filterData1.word2 & filterData0.word1)) {
        pairFlags |= PxPairFlag::eNOTIFY_TOUCH_FOUND;
    }

    return PxFilterFlag::eDEFAULT;
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

PxFilterFlags PhysicsMainFilterShader(PxFilterObjectAttributes attributes0, PxFilterData filterData0, PxFilterObjectAttributes attributes1, PxFilterData filterData1, PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize) {
    // generate contacts for all that were not filtered above
    pairFlags = PxPairFlag::eCONTACT_DEFAULT | PxPairFlag::eTRIGGER_DEFAULT | PxPairFlag::eNOTIFY_CONTACT_POINTS;
    return PxFilterFlag::eDEFAULT;
}


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
    PxU32 meshSize = 0;

    triMesh = PxCreateTriangleMesh(params, meshDesc, _physics->getPhysicsInsertionCallback());
    return triMesh;
    //triMesh->release();
}



PxConvexMesh* Physics::CreateConvexMesh(PxU32 numVertices, const PxVec3* vertices, PxU32 numTriangles, const PxU32* indices) {    
    PxConvexMeshDesc convexDesc;
    convexDesc.points.count = numVertices;
    convexDesc.points.stride = sizeof(PxVec3);
    convexDesc.points.data = vertices;
    convexDesc.flags = PxConvexFlag::eCOMPUTE_CONVEX;

    PxTolerancesScale scale;
    PxCookingParams params(scale);

    PxDefaultMemoryOutputStream buf;
    PxConvexMeshCookingResult::Enum result;
    if (!PxCookConvexMesh(params, convexDesc, buf, &result)) {
        return NULL;
    }
    PxDefaultMemoryInputData input(buf.getData(), buf.getSize());
    return _physics->createConvexMesh(input);
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

    _physics = PxCreatePhysics(PX_PHYSICS_VERSION, *_foundation, PxTolerancesScale(), true, _pvd);
    if (!_physics) {
        std::cout << "PxCreatePhysics failed!\n";
    }

    PxSceneDesc sceneDesc(_physics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
    _dispatcher = PxDefaultCpuDispatcherCreate(2);
    sceneDesc.cpuDispatcher = _dispatcher;
    //sceneDesc.filterShader = PxDefaultSimulationFilterShader;
   // sceneDesc.filterShader = FilterShaderExample;
    sceneDesc.filterShader = contactReportFilterShader;
    sceneDesc.simulationEventCallback = &_contactReportCallback;


    _scene = _physics->createScene(sceneDesc);
    _scene->setVisualizationParameter(PxVisualizationParameter::eSCALE, 1.0f);
    _scene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_SHAPES, 2.0f);


    PxPvdSceneClient* pvdClient = _scene->getScenePvdClient();
    if (pvdClient) {
        pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
        pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
        pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
    }
    _defaultMaterial = _physics->createMaterial(0.5f, 0.5f, 0.6f);

 /*   PxRigidStatic* groundPlane = PxCreatePlane(*_physics, PxPlane(0, 1, 0, -0.10f), *_defaultMaterial);
    _scene->addActor(*groundPlane);
    PxShape* shape;
    groundPlane->getShapes(&shape, 1);

    groundPlane->setActorFlag(PxActorFlag::eVISUALIZATION, false);

    PxFilterData filterData;
    filterData.word0 = RaycastGroup::RAYCAST_DISABLED; // must be disabled or it causes crash in scene::update when it tries to retrieve rigid body flags from this actor 
    filterData.word1 = CollisionGroup::ENVIROMENT_OBSTACLE;
    filterData.word2 =
        CollisionGroup::BULLET_CASING |
        CollisionGroup::GENERIC_BOUNCEABLE;
    shape->setQueryFilterData(filterData);
    shape->setSimulationFilterData(filterData); // sim is for ragz
    */

    //EnableRayCastingForShape(shape);
}

void Physics::StepPhysics(float deltaTime) {   
    _scene->simulate(deltaTime);
    _scene->fetchResults(true);
}

PxScene* Physics::GetScene() {
    return _scene;
}

PxPhysics* Physics::GetPhysics() {
    return _physics;
}

PxMaterial* Physics::GetDefaultMaterial() {
    return _defaultMaterial;
}

PxShape* Physics::CreateBoxShape(float width, float height, float depth, PxMaterial* material) {
    if (material == NULL) {
        material = _defaultMaterial;
    }
    return _physics->createShape(PxBoxGeometry(width, height, depth), *material);
}

PxShape* Physics::CreateShapeFromTriangleMesh(PxTriangleMesh* triangleMesh, PxMaterial* material, float scale) {
    if (material == NULL) {
        material = _defaultMaterial;
    }
    PxMeshGeometryFlags flags(~PxMeshGeometryFlag::eTIGHT_BOUNDS | ~PxMeshGeometryFlag::eDOUBLE_SIDED);
    PxTriangleMeshGeometry triGeometry(triangleMesh, PxMeshScale(scale), flags);
    return _physics->createShape(triGeometry, *material);
}

PxRigidDynamic* Physics::CreateRigidDynamic(Transform transform, PhysicsFilterData physicsFilterData, PxShape* shape) {
    PxFilterData filterData;
    filterData.word0 = (PxU32)physicsFilterData.raycastGroup;
    filterData.word1 = (PxU32)physicsFilterData.collisionGroup;
    filterData.word2 = (PxU32)physicsFilterData.collidesWith;
    shape->setQueryFilterData(filterData);       // ray casts
    shape->setSimulationFilterData(filterData);  // collisions
    PxQuat quat = Util::GlmQuatToPxQuat(glm::quat(transform.rotation));
    PxTransform trans = PxTransform(PxVec3(transform.position.x, transform.position.y, transform.position.z), quat);
    PxRigidDynamic* body = _physics->createRigidDynamic(trans);
    body->attachShape(*shape);
    PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);
    _scene->addActor(*body);
    return body;
}

std::vector<CollisionReport>& Physics::GetCollisions() {
    return _collisionReports;
}
void Physics::ClearCollisionList() {
    _collisionReports.clear();
}