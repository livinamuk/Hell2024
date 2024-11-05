#include "Brush.h"
#include "Defines.h"
#include "Util.hpp"

static float signed_distance(const glm::vec3& point, const csg::plane_t& plane);
static glm::vec3 project_point(const glm::vec3& point, const csg::plane_t& plane);
static csg::plane_t make_plane(const glm::vec3& point, const glm::vec3& normal);
static csg::plane_t transformed(const csg::plane_t& plane, const glm::mat4& transform);

Brush::Brush() {
    m_brush_t = nullptr;
}

void Brush::AddToWorld(csg::world_t& world) {
    m_brush_t = world.add();
    // Set type
    if (m_brushType == AIR_BRUSH) {
        m_brush_t->set_volume_operation(csg::make_fill_operation(AIR_BRUSH));
    }
    else if (m_brushType == SOLID_BRUSH) {
        m_brush_t->set_volume_operation(csg::make_fill_operation(SOLID_BRUSH));
    }
    // Create planes
    static std::vector<csg::plane_t> planes = {
        make_plane(glm::vec3(+1,0,0), glm::vec3(+1,0,0)),
        make_plane(glm::vec3(-1,0,0), glm::vec3(-1,0,0)),
        make_plane(glm::vec3(0,+1,0), glm::vec3(0,+1,0)),
        make_plane(glm::vec3(0,-1,0), glm::vec3(0,-1,0)),
        make_plane(glm::vec3(0,0,+1), glm::vec3(0,0,+1)),
        make_plane(glm::vec3(0,0,-1), glm::vec3(0,0,-1))
    };
    // Transform planes
    std::vector<csg::plane_t> transformed_planes;
    for (const csg::plane_t& plane : planes) {
        transformed_planes.push_back(transformed(plane, m_transform));
    }
    m_brush_t->set_planes(transformed_planes);
}

void Brush::SetBrushShape(BrushShape shape) {
    m_brushShape = shape;
}

void Brush::SetBrushType(BrushType type) {
    this->m_brushType = type;
}

void Brush::SetTransform(const glm::mat4& transform) {
    m_transform = transform;
    CreateCubeTriangles(RED);
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

void Brush::update_display_list() { // RENAME TO GET VERTICES AND ADD CHECK TO SEE IF m_brush_t is nullptr
    m_vertices.clear();
    m_vertices.reserve(80);
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

            // If it's a plane, remove the excess cube face vertices
            if (GetBrushShape() == BrushShape::PLANE) {
                glm::vec3 forward = Util::GetForwardVectorFromMatrix(m_transform);
                float angle = glm::acos(glm::dot(normal, forward)); 
                const float angleThreshold = glm::radians(1.0f);
                if (angle > angleThreshold) {
                    continue;
                }
            }
            // Otherwise get the vertices
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

void Brush::CreateCubeTriangles(glm::vec3 color) {
    g_cubeTriVertices.clear();
    float size = 1.0f;
    glm::vec3 FTL = m_transform * glm::vec4(-size, size, size, 1.0f);
    glm::vec3 FTR = m_transform * glm::vec4(size, size, size, 1.0f);
    glm::vec3 FBL = m_transform * glm::vec4(-size, -size, size, 1.0f);
    glm::vec3 FBR = m_transform * glm::vec4(size, -size, size, 1.0f);

    glm::vec3 BTL = m_transform * glm::vec4(-size, size, -size, 1.0f);
    glm::vec3 BTR = m_transform * glm::vec4(size, size, -size, 1.0f);
    glm::vec3 BBL = m_transform * glm::vec4(-size, -size, -size, 1.0f);
    glm::vec3 BBR = m_transform * glm::vec4(size, -size, -size, 1.0f);

    g_cubeTriVertices.push_back(Vertex(FTL, color));
    g_cubeTriVertices.push_back(Vertex(FTR, color));
    g_cubeTriVertices.push_back(Vertex(FBL, color));

    g_cubeTriVertices.push_back(Vertex(FBL, color));
    g_cubeTriVertices.push_back(Vertex(FTR, color));
    g_cubeTriVertices.push_back(Vertex(FBR, color));

    g_cubeTriVertices.push_back(Vertex(BTL, color));
    g_cubeTriVertices.push_back(Vertex(BTR, color));
    g_cubeTriVertices.push_back(Vertex(BBL, color));

    g_cubeTriVertices.push_back(Vertex(BBL, color));
    g_cubeTriVertices.push_back(Vertex(BTR, color));
    g_cubeTriVertices.push_back(Vertex(BBR, color));
}

std::vector<Vertex>& Brush::GetCubeTriangles() {
    return g_cubeTriVertices;
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

