#pragma once
#include "CSGCommon.h"
#include "RendererCommon.h"
#include "TinyCSG/tinycsg.hpp"
#include <span>

struct Brush {
public:
    Brush(csg::brush_t* brush);
    void SetBrushShape(BrushShape type);
    void SetBrushType(BrushType type);
    void SetTransform(const glm::mat4& transform);
    const BrushType& GetBrushType();
    const BrushShape& GetBrushShape();
    const glm::mat4& GetTransform();
    void update_display_list();
    std::vector<CSGVertex> m_vertices;
private:
    glm::mat4 m_transform;
    csg::brush_t* m_brush_t;
    BrushType m_brushType;
    BrushShape m_brushShape = BrushShape::CUBE;
};
