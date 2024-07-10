#include "csg.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include "../Common.h"
#include "../Util.hpp"
#include "../API/OpenGL/GL_backEnd.h"
#include "../BackEnd/Backend.h"
#include "../Core/AssetManager.h"
#include "../Game/Scene.h"

namespace CSG {

    std::vector<CSGObject> g_objects;
    csg::world_t g_world;
    std::vector<Vertex> g_vertices;
    bool g_sceneDirty = true;

    void Init() {
        csg::volume_t void_volume = g_world.get_void_volume();
        g_world.set_void_volume(AIR);
    }

    void Update() {


    }

    bool GeometryExists() {
        return g_vertices.size();
    }

    glm::vec2 CalculateUV(const glm::vec3& vertexPosition, const glm::vec3& faceNormal, const glm::vec3& origin) {
        glm::vec2 uv;
        // Find the dominant axis of the face normal
        glm::vec3 absNormal = glm::abs(faceNormal);
        if (absNormal.x > absNormal.y && absNormal.x > absNormal.z) {
            // Dominant axis is X, project onto YZ plane
            uv.y = (vertexPosition.y - origin.y) / absNormal.x;
            uv.x = (vertexPosition.z - origin.z) / absNormal.x;
            uv.y = 1.0f - uv.y;
            if (faceNormal.x > 0) {
                uv.x = 1.0f - uv.x;
            }
        }
        else if (absNormal.y > absNormal.x && absNormal.y > absNormal.z) {
            // Dominant axis is Y, project onto XZ plane
            uv.y = (vertexPosition.x - origin.x) / absNormal.y;
            uv.x = (vertexPosition.z - origin.z) / absNormal.y;
            uv.y = 1.0f - uv.y;
            if (faceNormal.y < 0) {
                uv.y = 1.0f - uv.y;
            }
        }
        else {
            // Dominant axis is Z, project onto XY plane
            uv.x = (vertexPosition.x - origin.x) / absNormal.z;
            uv.y = (vertexPosition.y - origin.y) / absNormal.z;
            uv.y = 1.0f - uv.y;
            if (faceNormal.z < 0) {
                uv.x = 1.0f - uv.x;
            }
        }
        return uv;
    }

    void CleanUp() {
        for (CSGObject& cube : g_objects) {
            cube.CleanUpPhysicsObjects();
        }
        g_objects.clear();
    }

    void Build() {

        CleanUp();

        for (int i = 0; i < Scene::g_cubeVolumesAdditive.size(); i++) {
            CubeVolume& cubeVolume = Scene::g_cubeVolumesAdditive[i];
            CSGObject& csg = g_objects.emplace_back();
            csg.m_transform = cubeVolume.GetTransform();
            csg.m_type = CSGType::ADDITIVE;
            csg.m_materialIndex = cubeVolume.materialIndex;
            csg.m_textureScale = cubeVolume.textureScale;
            csg.m_parentIndex = i;
        }

        for (int i = 0; i < Scene::g_cubeVolumesSubtractive.size(); i++) {
            CubeVolume& cubeVolume = Scene::g_cubeVolumesSubtractive[i];
            CSGObject& csg = g_objects.emplace_back();
            csg.m_transform = cubeVolume.GetTransform();
            csg.m_type = CSGType::SUBTRACTIVE;
            csg.m_materialIndex = cubeVolume.materialIndex;
            csg.m_textureScale = cubeVolume.textureScale;
            csg.m_parentIndex = i;
        }

        for (Door& door : Scene::g_doors) {
            CSGObject& csg = g_objects.emplace_back();
            csg.m_transform.position = door.m_position + glm::vec3(0, DOOR_HEIGHT / 2, 0);
            csg.m_transform.rotation = glm::vec3(0, door.m_rotation, 0);
            csg.m_transform.scale = glm::vec3(0.2f, DOOR_HEIGHT, 0.8f);
            csg.m_type = CSGType::DOOR;
            csg.m_materialIndex = AssetManager::GetMaterialIndex("FloorBoards"); // add this to the door object somehow
            csg.m_textureScale = 0.5f;                                           // add this to the door object somehow
        }


        for (Window& window: Scene::g_windows) {
            CSGObject& csg = g_objects.emplace_back();
            csg.m_transform.position = window.GetPosition() + glm::vec3(0, 1.5f, 0);
            csg.m_transform.scale = glm::vec3(0.8f, 1.3f, 0.2f);
            csg.m_transform.rotation = window.rotation;
            csg.m_type = CSGType::SUBTRACTIVE;
            csg.m_materialIndex = AssetManager::GetMaterialIndex("FloorBoards"); // add this to the door object somehow
            csg.m_textureScale = 0.5f;                                           // add this to the door object somehow
        }


        for (int i = 0; i < g_objects.size(); i++) {
            CSGObject& cube = g_objects[i];
            Transform transform = cube.m_transform;
            transform.scale *= glm::vec3(0.5f);
            csg::brush_t* brush = g_world.add();
            brush->userdata = cube_brush_userdata_t(brush);
            cube_brush_userdata_t* userdata = any_cast<cube_brush_userdata_t>(&brush->userdata);
            if (cube.m_type == CSGType::ADDITIVE) {
                userdata->set_brush_type(SOLID_BRUSH);
            }
            else if (cube.m_type == CSGType::SUBTRACTIVE) {
                userdata->set_brush_type(AIR_BRUSH);
            }
            else if (cube.m_type == CSGType::DOOR) {
                userdata->set_brush_type(AIR_BRUSH);
            }
            else if (cube.m_type == CSGType::WINDOW) {
                userdata->set_brush_type(AIR_BRUSH);
            }
            userdata->set_transform(transform.to_mat4());
            userdata->set_parentIndex(i);
        }

        // Calculate vertices
        g_vertices.clear();
        int cubeIndex = 0;
        int baseVertex = 0;
        auto rebuilt = g_world.rebuild();

        for (csg::brush_t* b : rebuilt) {

            cube_brush_userdata_t* brush = any_cast<cube_brush_userdata_t>(&b->userdata);
            brush->update_display_list();

            CSGObject& cube = g_objects[brush->m_index];

            int vertexCount = 0;
            for (Vertex& vertex : brush->m_vertices) {
                glm::vec3 origin = brush->m_origin;
                origin = glm::vec3(0);
                vertex.uv = CalculateUV(vertex.position, vertex.normal, origin);
                vertex.uv *= cube.m_textureScale;
                g_vertices.push_back(vertex);
                vertexCount++;
            }
            cube.m_baseVertex = baseVertex;
            cube.m_vertexCount = vertexCount;
            cubeIndex++;
            baseVertex += vertexCount;
        }

        // Calculate tangents
        for (int i = 0; i < g_vertices.size(); i += 3) {
            Util::SetNormalsAndTangentsFromVertices(&g_vertices[i], &g_vertices[i+1], &g_vertices[i+2]);
        }

        // Remove all brushes
        csg::brush_t* brush = g_world.first();
        while (brush) {
            g_world.remove(brush);
            brush = g_world.first();
        }

        for (CSGObject& cube : g_objects) {
            cube.CreatePhysicsObjectFromVertices();
        }

        g_sceneDirty = true;
        std::cout << "Built constructive solid geometry\n";

        if (g_sceneDirty && g_vertices.size()) {
            if (BackEnd::GetAPI() == API::OPENGL) {
                OpenGLBackEnd::UploadConstructiveSolidGeometry(g_vertices);
            }
            else if (BackEnd::GetAPI() == API::VULKAN) {

            }
            g_sceneDirty = false;
        }

        for (Light& light : Scene::g_lights) {
            light.isDirty = true;
        }
    }

    std::vector<Vertex>& GetVertices() {
        return g_vertices;
    }

    std::vector<CSGObject>& GetCSGObjects() {
        return g_objects;
    }

    void SetDirtyFlag(bool value) {
        g_sceneDirty = value;
    }

    bool  IsDirty() {
        return g_sceneDirty;
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


void cube_brush_userdata_t::set_parentIndex(uint32_t index) {
    m_index = index;
}

void cube_brush_userdata_t::set_brush_type(brush_type_t type) {
    this->type = type;
    switch (type) {
    case AIR_BRUSH: brush->set_volume_operation(csg::make_fill_operation(AIR)); break;
    case SOLID_BRUSH: brush->set_volume_operation(csg::make_fill_operation(SOLID)); break;
    default:;
    }
}

brush_type_t cube_brush_userdata_t::get_brush_type() {
    return type;
}

void cube_brush_userdata_t::set_transform(const glm::mat4& transform) {
    static std::vector<csg::plane_t> cube_planes = {
        make_plane(glm::vec3(+1,0,0), glm::vec3(+1,0,0)),
        make_plane(glm::vec3(-1,0,0), glm::vec3(-1,0,0)),
        make_plane(glm::vec3(0,+1,0), glm::vec3(0,+1,0)),
        make_plane(glm::vec3(0,-1,0), glm::vec3(0,-1,0)),
        make_plane(glm::vec3(0,0,+1), glm::vec3(0,0,+1)),
        make_plane(glm::vec3(0,0,-1), glm::vec3(0,0,-1))
    };
    this->transform = transform;

    std::vector<csg::plane_t> transformed_planes;
    for (const csg::plane_t& plane : cube_planes)
        transformed_planes.push_back(transformed(plane, transform));

    brush->set_planes(transformed_planes);
}

const glm::mat4& cube_brush_userdata_t::get_transform() {
    return transform;
}

void cube_brush_userdata_t::update_display_list() {

    m_vertices.clear();
    auto faces = brush->get_faces();

    for (csg::face_t& face : faces) {

        for (csg::fragment_t& fragment : face.fragments) {

            if (fragment.back_volume == fragment.front_volume) {
                continue;
            }

            glm::vec3 normal = face.plane->normal;
            bool flip_face = fragment.back_volume == AIR;

            if (flip_face) {
                normal = -normal;
            }

            std::vector<csg::triangle_t> tris = triangulate(fragment);

            for (csg::triangle_t& tri : tris) {

                if (flip_face) {
                    m_vertices.push_back(Vertex(fragment.vertices[tri.i].position, normal));
                    m_vertices.push_back(Vertex(fragment.vertices[tri.k].position, normal));
                    m_vertices.push_back(Vertex(fragment.vertices[tri.j].position, normal));
                }
                else {
                    m_vertices.push_back(Vertex(fragment.vertices[tri.i].position, normal));
                    m_vertices.push_back(Vertex(fragment.vertices[tri.j].position, normal));
                    m_vertices.push_back(Vertex(fragment.vertices[tri.k].position, normal));
                }
            }
        }
    }
}

cube_brush_userdata_t::cube_brush_userdata_t(csg::brush_t* _brush) : brush(brush) {
    brush = _brush;
}


std::span<Vertex> CSGObject::GetVerticesSpan() {
    return std::span<Vertex>(CSG::g_vertices.data() + m_baseVertex, m_vertexCount);
}

void CSGObject::CleanUpPhysicsObjects() {

    if (m_triangleMesh) {
        m_triangleMesh->release();
    }
    if (m_pxRigidStatic) {
        if (m_pxRigidStatic->userData) {
            auto* data = static_cast<PhysicsObjectData*>(m_pxRigidStatic->userData);
            delete data;
        }
        Physics::GetScene()->removeActor(*m_pxRigidStatic);
        m_pxRigidStatic->release();
        m_pxRigidStatic = nullptr;
    }
    if (m_pxShape) {
        m_pxShape->release();
    }
}

void CSGObject::CreatePhysicsObjectFromVertices() {

    CleanUpPhysicsObjects();

    if (m_type == CSGType::ADDITIVE && m_vertexCount > 0) {

        // Create triangle mesh
        std::vector<PxVec3> pxvertices;
        std::vector<unsigned int> pxindices;
        for (Vertex& vertex : GetVerticesSpan()) {
            pxvertices.push_back(PxVec3(vertex.position.x, vertex.position.y, vertex.position.z));
        }
        for (int i = 0; i < m_vertexCount; i++) {
            pxindices.push_back(i);
        }

        m_triangleMesh = Physics::CreateTriangleMesh(pxvertices.size(), pxvertices.data(), pxindices.size() / 3, pxindices.data());

        /*
        PhysicsFilterData filterData;
        filterData.raycastGroup = RAYCAST_ENABLED;
        filterData.collisionGroup = CollisionGroup::NO_COLLISION;
        filterData.collidesWith = CollisionGroup::NO_COLLISION;
        PxShapeFlags shapeFlags(PxShapeFlag::eSCENE_QUERY_SHAPE);*/

        PhysicsFilterData filterData;
        filterData.raycastGroup = RAYCAST_ENABLED;
        filterData.collisionGroup = ENVIROMENT_OBSTACLE;
        filterData.collidesWith = (CollisionGroup)(GENERIC_BOUNCEABLE | BULLET_CASING | PLAYER | RAGDOLL);
        PxShapeFlags shapeFlags(PxShapeFlag::eSCENE_QUERY_SHAPE | PxShapeFlag::eSIMULATION_SHAPE);

        m_pxShape = Physics::CreateShapeFromTriangleMesh(m_triangleMesh, shapeFlags);
        m_pxRigidStatic = Physics::CreateRigidStatic(Transform(), filterData, m_pxShape);
        m_pxRigidStatic->userData = new PhysicsObjectData(PhysicsObjectType::CSG_OBJECT_ADDITIVE, this);

    }
}