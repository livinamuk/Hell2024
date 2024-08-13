#pragma once
#include "TinyCSG/tinycsg.hpp"
#include "../Renderer/RendererCommon.h"
#include "../Physics/RigidStatic.hpp"
#include <span>

static constexpr csg::volume_t AIR = 0;
static constexpr csg::volume_t SOLID = 1;

enum brush_type_t {
    AIR_BRUSH,
    SOLID_BRUSH
};

struct cube_brush_userdata_t {
public:
    void set_parentIndex(uint32_t index);
    void set_brush_type(brush_type_t type);
    brush_type_t get_brush_type();
    void set_transform(const glm::mat4& transform);
    const glm::mat4& get_transform();
    void update_display_list();
    cube_brush_userdata_t(csg::brush_t* brush);

    std::vector<CSGVertex> m_vertices;
    glm::vec3 m_color;
    glm::vec3 m_origin;
    uint32_t m_index;

private:
    glm::mat4 transform;
    csg::brush_t* brush;
    brush_type_t type;
};

enum class CSGType {
    ADDITIVE,
    SUBTRACTIVE,
    DOOR,
    WINDOW
};

struct CSGObject {

    Transform m_transform;;
    CSGType m_type;
    uint32_t m_baseVertex = 0;
    uint32_t m_indexCount = 0;
    uint32_t m_vertexCount = 0;
    uint32_t m_baseIndex = 0;
    uint32_t m_materialIndex = 0;
    uint32_t m_parentIndex = 0;
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
}