#include "Physics.h"
#include <vector>
#include "iostream"
#include "../Common.h"
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
PxPhysics* _physics = NULL;
PxDefaultCpuDispatcher* _dispatcher = NULL;
PxScene* _scene = NULL;
PxMaterial* _defaultMaterial = NULL;
ContactReportCallback   _contactReportCallback;
PxRigidStatic* _groundPlane = NULL;
PxShape* _groundShape = NULL;

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

    triMesh = PxCreateTriangleMesh(params, meshDesc, _physics->getPhysicsInsertionCallback());
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

    // Character controller shit
    _characterControllerManager = PxCreateControllerManager(*_scene);




    //std::cout << "creating ground plane..\n";
    _groundPlane = PxCreatePlane(*_physics, PxPlane(0, 1, 0, 0.0f), *_defaultMaterial);
    _scene->addActor(*_groundPlane);
    _groundPlane->getShapes(&_groundShape, 1);
    PxFilterData filterData;
    filterData.word0 = RaycastGroup::RAYCAST_DISABLED; // must be disabled or it causes crash in scene::update when it tries to retrieve rigid body flags from this actor 
    filterData.word1 = CollisionGroup::ENVIROMENT_OBSTACLE;
    filterData.word2 = CollisionGroup::BULLET_CASING | CollisionGroup::GENERIC_BOUNCEABLE | CollisionGroup::PLAYER;
    _groundShape->setQueryFilterData(filterData);
    _groundShape->setSimulationFilterData(filterData); // sim is for ragz   
    //std::cout << "created ground plane..\n";

    //EnableRayCastingForShape(shape);
}

/*
void Physics::RemoveGroundPlaneFilterData() {
    PxFilterData filterData;
    filterData.word0 = 0;
    filterData.word1 = 0;
    filterData.word2 = 0;
    _groundShape->setQueryFilterData(filterData);
    _groundShape->setSimulationFilterData(filterData);
}

void Physics::AddGroundPlaneFilterData() {
    PxFilterData filterData;
    filterData.word0 = RaycastGroup::RAYCAST_DISABLED; // must be disabled or it causes crash in scene::update when it tries to retrieve rigid body flags from this actor
    filterData.word1 = CollisionGroup::ENVIROMENT_OBSTACLE;
    filterData.word2 = CollisionGroup::BULLET_CASING | CollisionGroup::GENERIC_BOUNCEABLE |	CollisionGroup::PLAYER;
    _groundShape->setQueryFilterData(filterData);
    _groundShape->setSimulationFilterData(filterData); // sim is for ragz
}*/


void Physics::StepPhysics(float deltaTime) {
    //float maxSimulateTime = (1.0f / 60.0f) * 4.0f;
    //_scene->simulate(std::min(deltaTime, maxSimulateTime));

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

PxShape* Physics::CreateBoxShape(float width, float height, float depth, Transform shapeOffset, PxMaterial* material) {
    if (material == NULL) {
        material = _defaultMaterial;
    }
    PxShape* shape = _physics->createShape(PxBoxGeometry(width, height, depth), *material, true);
    PxMat44 localShapeMatrix = Util::GlmMat4ToPxMat44(shapeOffset.to_mat4());
    PxTransform localShapeTransform(localShapeMatrix);
    shape->setLocalPose(localShapeTransform);
    return shape;
}

PxShape* Physics::CreateShapeFromTriangleMesh(PxTriangleMesh* triangleMesh, PxShapeFlags shapeFlags2, PxMaterial* material, float scale) {
    if (material == NULL) {
        material = _defaultMaterial;
    }
    PxMeshGeometryFlags flags(~PxMeshGeometryFlag::eTIGHT_BOUNDS | ~PxMeshGeometryFlag::eDOUBLE_SIDED);
    PxTriangleMeshGeometry geometry(triangleMesh, PxMeshScale(scale), flags);


    PxShapeFlags shapeFlags(PxShapeFlag::eSCENE_QUERY_SHAPE); // Most importantly NOT eSIMULATION_SHAPE. PhysX does not allow for tri mesh.
    return _physics->createShape(geometry, *material, shapeFlags);
}

PxShape* Physics::CreateShapeFromConvexMesh(PxConvexMesh* convexMesh, PxMaterial* material, float /*scale*/) {
    if (material == NULL) {
        material = _defaultMaterial;
    }
    PxConvexMeshGeometryFlags flags(~PxConvexMeshGeometryFlag::eTIGHT_BOUNDS);
    PxConvexMeshGeometry geometry(convexMesh, PxMeshScale(), flags);
    return _physics->createShape(geometry, *material);
}

PxRigidDynamic* Physics::CreateRigidDynamic(Transform transform, PhysicsFilterData physicsFilterData, PxShape* shape, Transform shapeOffset) {

    PxQuat quat = Util::GlmQuatToPxQuat(glm::quat(transform.rotation));
    PxTransform trans = PxTransform(PxVec3(transform.position.x, transform.position.y, transform.position.z), quat);
    PxRigidDynamic* body = _physics->createRigidDynamic(trans);

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
    _scene->addActor(*body);
    return body;
}

PxRigidStatic* Physics::CreateRigidStatic(Transform transform, PhysicsFilterData physicsFilterData, PxShape* shape, Transform shapeOffset) {

    PxQuat quat = Util::GlmQuatToPxQuat(glm::quat(transform.rotation));
    PxTransform trans = PxTransform(PxVec3(transform.position.x, transform.position.y, transform.position.z), quat);
    PxRigidStatic* body = _physics->createRigidStatic(trans);

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
    _scene->addActor(*body);
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
    PxRigidDynamic* body = _physics->createRigidDynamic(transform);
    body->attachShape(*shape);
    PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);
    _scene->addActor(*body);
    return body;
}

PxRigidDynamic* Physics::CreateRigidDynamic(glm::mat4 matrix, bool kinematic) {
    PxMat44 mat = Util::GlmMat4ToPxMat44(matrix);
    PxTransform transform(mat);
    PxRigidDynamic* body = _physics->createRigidDynamic(transform);
    body->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, kinematic);
    _scene->addActor(*body);
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
