#include "Ladder.h"
#include "../../Core/AssetManager.h"
#include "../../Renderer/RendererUtil.hpp"
#include <limits>

void Ladder::Init(LadderCreateInfo createInfo) {
    CleanUp();

    // Create render items
    if (createInfo.yCount > 0) {
        m_transforms.clear();
        for (int i = 0; i < createInfo.yCount; i++) {
            Transform& transform = m_transforms.emplace_back();
            transform.position = createInfo.position - glm::vec3(0.0f, i * 2.44f, 0.0f);
            transform.rotation.y = createInfo.rotation;
        }
        m_forward = m_transforms[0].to_forward_vector();
    }
    CreateRenderItems();

    // Compute AABB
    glm::vec3 boundsMin = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 boundsMax = glm::vec3(std::numeric_limits<float>::lowest());
    for (const RenderItem3D& renderItem : m_renderItems) {
        boundsMin = glm::min(boundsMin, renderItem.aabbMin);
        boundsMax = glm::max(boundsMax, renderItem.aabbMax);
    }
    float shrink = 0.125;
    boundsMin.x += shrink;
    boundsMin.z += shrink;
    boundsMax.x -= shrink;
    boundsMax.z -= shrink;
    boundsMax.y += 0.9;
    m_overlapHitAABB = AABB(boundsMin, boundsMax);

    // Create overlap shape
    //Transform overlapTransform;
    //overlapTransform.position = m_aabb.GetCenter();
    //PhysicsFilterData filterData;
    //filterData.raycastGroup = RaycastGroup::RAYCAST_DISABLED;
    //filterData.collisionGroup = CollisionGroup::LADDER;
    //filterData.collidesWith = CollisionGroup::NO_COLLISION;
    //m_pxShape = Physics::CreateBoxShape(m_aabb.GetExtents().x, m_aabb.GetExtents().y, m_aabb.GetExtents().z);
    //m_pxRigidStatic = Physics::CreateRigidStatic(overlapTransform, filterData, m_pxShape);
    //UpdatePhysXPointer();
}

void Ladder::Update(float deltaTime) {
    // Nothing yet
}

void Ladder::CleanUp() {
    Physics::Destroy(m_pxRigidStatic);
    Physics::Destroy(m_pxShape);
}

void Ladder::CreateRenderItems() {
    static Model* model = AssetManager::GetModelByName("Ladder");
    static Material* material = AssetManager::GetMaterialByIndex(AssetManager::GetMaterialIndex("Ladder"));
    static uint32_t meshIndex = model->GetMeshIndices()[0];
    static Mesh* mesh = AssetManager::GetMeshByIndex(meshIndex);

    m_renderItems.clear();
    for (Transform& transform : m_transforms) {
        RenderItem3D& renderItem = m_renderItems.emplace_back();
        renderItem.vertexOffset = mesh->baseVertex;
        renderItem.indexOffset = mesh->baseIndex;
        renderItem.modelMatrix = transform.to_mat4();
        renderItem.inverseModelMatrix = inverse(renderItem.modelMatrix);
        renderItem.meshIndex = meshIndex;
        renderItem.normalMapTextureIndex = material->_normal;
        renderItem.castShadow = true;            
        renderItem.baseColorTextureIndex = material->_basecolor;
        renderItem.rmaTextureIndex = material->_rma;
        renderItem.normalMapTextureIndex = material->_normal;
        RendererUtil::CalculateAABB(renderItem);
    }
}

std::vector<RenderItem3D>& Ladder::GetRenderItems() {
    return m_renderItems;
}

glm::vec3 Ladder::GetForwardVector() {
    return m_forward;
}

void Ladder::UpdatePhysXPointer() {
    if (m_pxRigidStatic) {
        if (m_pxRigidStatic->userData) {
            delete static_cast<PhysicsObjectData*>(m_pxRigidStatic->userData);
        }
        m_pxRigidStatic->userData = new PhysicsObjectData(ObjectType::LADDER, this);
    }
}

float Ladder::GetTopHeight() {
    if (m_transforms.empty()) {
        return 0;
    }
    else {
        return m_transforms[0].position.y;
    }
}

const AABB& Ladder::GetOverlapHitBoxAABB() {
    return m_overlapHitAABB;
}