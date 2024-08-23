#include "Brush.h"

static float signed_distance(const glm::vec3& point, const csg::plane_t& plane);
static glm::vec3 project_point(const glm::vec3& point, const csg::plane_t& plane);
static csg::plane_t make_plane(const glm::vec3& point, const glm::vec3& normal);
static csg::plane_t transformed(const csg::plane_t& plane, const glm::mat4& transform);

Brush::Brush(csg::brush_t* brush) {
    m_brush_t = brush;
}

void Brush::SetBrushShape(BrushShape shape) {
    m_brushShape = shape;
}

void Brush::SetBrushType(BrushType type) {
    if (type == AIR_BRUSH) {
        m_brush_t->set_volume_operation(csg::make_fill_operation(AIR_BRUSH));
    }
    else if (type == SOLID_BRUSH) {
        m_brush_t->set_volume_operation(csg::make_fill_operation(SOLID_BRUSH));
    }
    this->m_brushType = type;
}

void Brush::SetTransform(const glm::mat4& transform) {
    m_transform = transform;
    static std::vector<csg::plane_t> planes;
    if (m_brushShape == BrushShape::CUBE) {
        planes = {
            make_plane(glm::vec3(+1,0,0), glm::vec3(+1,0,0)),
            make_plane(glm::vec3(-1,0,0), glm::vec3(-1,0,0)),
            make_plane(glm::vec3(0,+1,0), glm::vec3(0,+1,0)),
            make_plane(glm::vec3(0,-1,0), glm::vec3(0,-1,0)),
            make_plane(glm::vec3(0,0,+1), glm::vec3(0,0,+1)),
            make_plane(glm::vec3(0,0,-1), glm::vec3(0,0,-1))
        };
    }
    else if (m_brushShape == BrushShape::PLANE) {
        planes = {
            make_plane(glm::vec3(+1,0,0), glm::vec3(+1,0,0)),
            make_plane(glm::vec3(-1,0,0), glm::vec3(-1,0,0)),
            make_plane(glm::vec3(0,+1,0), glm::vec3(0,+1,0)),
            make_plane(glm::vec3(0,-1,0), glm::vec3(0,-1,0)),
            make_plane(glm::vec3(0,0,+1), glm::vec3(0,0,+1)),
            make_plane(glm::vec3(0,0,-1), glm::vec3(0,0,-1))
        };
    }
    std::vector<csg::plane_t> transformed_planes;
    for (const csg::plane_t& plane : planes) {
        transformed_planes.push_back(transformed(plane, transform));
    }
    m_brush_t->set_planes(transformed_planes);
}

const glm::mat4& Brush::GetTransform() {
    return m_transform;
}

const BrushShape& Brush::GetBrushShape() {
    return m_brushShape;
}
const BrushType& Brush::GetBrushType() {
    return m_brushType;
}

void Brush::update_display_list() {
    m_vertices.clear();
    m_vertices.reserve(512);
    auto faces = m_brush_t->get_faces();
    for (csg::face_t& face : faces) {
        for (csg::fragment_t& fragment : face.fragments) {
            if (fragment.back_volume == fragment.front_volume) {
                continue;
            }
            glm::vec3 normal = face.plane->normal;
            bool flip_face = fragment.back_volume == AIR_BRUSH;
            if (flip_face) {
                normal = -normal;
            }
            std::vector<csg::triangle_t> tris = triangulate(fragment);
            for (csg::triangle_t& tri : tris) {
                if (flip_face) {
                    m_vertices.push_back(CSGVertex(fragment.vertices[tri.i].position, normal));
                    m_vertices.push_back(CSGVertex(fragment.vertices[tri.k].position, normal));
                    m_vertices.push_back(CSGVertex(fragment.vertices[tri.j].position, normal));
                }
                else {
                    m_vertices.push_back(CSGVertex(fragment.vertices[tri.i].position, normal));
                    m_vertices.push_back(CSGVertex(fragment.vertices[tri.j].position, normal));
                    m_vertices.push_back(CSGVertex(fragment.vertices[tri.k].position, normal));
                }
            }
        }
    }
}

static float signed_distance(const glm::vec3& point, const csg::plane_t& plane) {
    return dot(plane.normal, point) + plane.offset;
}

static glm::vec3 project_point(const glm::vec3& point, const csg::plane_t& plane) {
    return point - signed_distance(point, plane) * plane.normal;
}

static csg::plane_t make_plane(const glm::vec3& point, const glm::vec3& normal) {
    return csg::plane_t{ normal, -dot(point, normal) };
}

static csg::plane_t transformed(const csg::plane_t& plane, const glm::mat4& transform) {
    glm::vec3 origin_transformed = glm::vec3(transform * glm::vec4(project_point(glm::vec3(0, 0, 0), plane), 1.0f));
    glm::vec3 normal_transformed = glm::normalize(glm::vec3(transpose(glm::inverse(transform)) * glm::vec4(plane.normal, 0.0f)));
    csg::plane_t transformed_plane = make_plane(origin_transformed, normal_transformed);
    return transformed_plane;
}

