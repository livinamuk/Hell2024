#pragma once
#include "RendererCommon.h"
#include "TinyCSG/tinycsg.hpp"
#include "../Physics/RigidStatic.hpp"
#include <span>
#include "CSGCommon.h"
#include "brush.h"

static constexpr csg::volume_t AIR = 0;
static constexpr csg::volume_t SOLID = 1;

struct CSGObject {

    Transform m_transform;;
    ObjectType m_parentObjectType;
    CSGType m_type;
    uint32_t m_baseVertex = 0;
    uint32_t m_indexCount = 0;
    uint32_t m_vertexCount = 0;
    uint32_t m_baseIndex = 0;
    uint32_t m_materialIndex = 0;
    uint32_t m_parentIndex = 0;
    uint32_t m_localIndex = 0;
    uint32_t m_blasIndex = 0;
    float m_textureScale = 1.0f;
    float m_textureOffsetX = 0.0f;
    float m_textureOffsetY = 0.0f;
    PxTriangleMesh* m_triangleMesh = nullptr;
    PxRigidStatic* m_pxRigidStatic = nullptr;
    PxShape* m_pxShape = nullptr;
    AABB m_aabb;
    csg::brush_t* m_brush;
    glm::mat4 m_parentVolumeNormalMatrix = glm::mat4(1);
    bool m_disableRendering = false;

    std::span<CSGVertex> GetVerticesSpan();
    std::span<uint32_t> GetIndicesSpan();
    void CreatePhysicsObjectFromVertices();
    void CleanUpPhysicsObjects();
};


namespace CSG {
    void Init();
    void Update();
    void Build();
    bool GeometryExists();
    std::vector<CSGVertex>& GetVertices();
    std::vector<glm::vec3>& GetNavMeshVertices();
    std::vector<uint32_t>& GetIndices();
    std::vector<CSGObject>& GetCSGObjects();
    std::span<CSGVertex> GetRangedVerticesSpan(uint32_t baseVertex, uint32_t vertexCount);
    std::span<uint32_t> GetRangedIndicesSpan(uint32_t baseIndex, uint32_t indexCount);
    uint32_t GetBaseCSGVertex();
    inline std::vector<Brush> g_subtractiveBrushes;

}