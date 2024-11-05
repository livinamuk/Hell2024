#pragma once
#include "CSGCommon.h"
#include "RendererCommon.h"
#include "TinyCSG/tinycsg.hpp"
#include <span>

struct Brush {
public:
    Brush();
    void AddToWorld(csg::world_t& world);
    void SetBrushShape(BrushShape type);
    void SetBrushType(BrushType type);
    void SetTransform(const glm::mat4& transform);
    const BrushType& GetBrushType();
    const BrushShape& GetBrushShape();
    const glm::mat4& GetTransform();
    void update_display_list();
    std::vector<CSGVertex> m_vertices;

    void CreateCubeTriangles(glm::vec3 color);
    std::vector<Vertex>& GetCubeTriangles();
private:
    csg::brush_t* m_brush_t;
    BrushType m_brushType = BrushType::SOLID_BRUSH;
    BrushShape m_brushShape = BrushShape::CUBE;
    std::vector<Vertex> g_cubeTriVertices;
    glm::mat4 m_transform;
};
