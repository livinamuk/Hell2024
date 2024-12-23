#include "csg.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <future>
#include <vector>
#include "../Util.hpp"
#include "../API/OpenGL/GL_backEnd.h"
#include "../BackEnd/Backend.h"
#include "../Core/AssetManager.h"
#include "../Game/Scene.h"
#include "../Pathfinding/Pathfinding2.h"
#include "../Renderer/GlobalIllumination.h"
#include "../Timer.hpp"

namespace CSG {

    std::vector<CSGObject> g_objects;
    std::vector<CSGVertex> g_vertices;
    std::vector<uint32_t> g_indices;
    bool g_sceneDirty = true;
    uint32_t g_baseCSGVertex = 0;
    std::vector<glm::vec3> g_navMeshVertices;


    //std::vector<CSGObject> g_subtractiveObjects; // used for plane world

    void UpdateDisplayLists();

    void Init() {

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
        g_navMeshVertices.clear();
    }

    enum class CSGObjecType { CUBE, WALL_PLANE, FLOOR_PLANE, CEILING_PLANE };

    struct CSGNew {
        std::vector<CSGVertex> m_vertices;
        std::vector<uint32_t> m_indices;
        uint32_t m_materialIndex = 0;
        uint32_t m_parentIndex = 0;
        uint32_t m_localIndex = 0;
        AABB m_aabb;
        CSGObjecType m_type = CSGObjecType::CUBE;
    };
    std::vector<CSGNew> g_csgNewObjects;

    void CreateCSGObjectFromBrush(Brush& brush, CSGObjecType csgObjectType, int parentIndex, int materialIndex, float textureScale, float textureOffsetX, float textureOffsetY) {
        //Timer timer("CreateCSGObjectFromBrush()");
        // Create new world
        csg::world_t world;
        world.set_void_volume(AIR);

        // Add the brush
        brush.AddToWorld(world);

        // Find intersecting subtractive brushes (TODO: check for intersection!!!)
        std::vector<Brush*> subtractiveBrushPointers;
        for (Brush& brush : g_subtractiveBrushes) {
            brush.AddToWorld(world);
            subtractiveBrushPointers.push_back(&brush);
        }
        
        // Build the world
        world.rebuild();

        // Get vertices
        CSGNew& csgNew = g_csgNewObjects.emplace_back();
        csgNew.m_type = csgObjectType;
        csgNew.m_parentIndex = parentIndex;
        csgNew.m_materialIndex = materialIndex;
        brush.update_display_list();
        csgNew.m_vertices.insert(csgNew.m_vertices.end(), brush.m_vertices.begin(), brush.m_vertices.end());

        // If a cube, get the subtracted vertices
        if (brush.GetBrushShape() == BrushShape::CUBE) {
            for (Brush* subtractiveBrush : subtractiveBrushPointers) {
                subtractiveBrush->update_display_list();
                csgNew.m_vertices.insert(csgNew.m_vertices.end(), subtractiveBrush->m_vertices.begin(), subtractiveBrush->m_vertices.end());
            }
        }

        // Calculate UVS and AABB
        glm::vec3 boundsMin = glm::vec3(1e30f);
        glm::vec3 boundsMax = glm::vec3(-1e30f);
        int index = 0;
        for (CSGVertex& vertex : csgNew.m_vertices) {
            glm::vec3 origin = glm::vec3(0, 0, 0);
            origin = glm::vec3(0);
            vertex.uv = CalculateUV(vertex.position, vertex.normal, origin);
            vertex.uv *= textureScale;
            vertex.uv.x += textureOffsetX;
            vertex.uv.y += textureOffsetY;
            boundsMin = Util::Vec3Min(boundsMin, vertex.position);
            boundsMax = Util::Vec3Max(boundsMax, vertex.position);
            csgNew.m_indices.push_back(index++);
        }
        csgNew.m_aabb = AABB(boundsMin, boundsMax);

        // Normals and tangents
        for (int i = 0; i < csgNew.m_vertices.size(); i += 3) {
            Util::SetNormalsAndTangentsFromVertices(&csgNew.m_vertices[i], &csgNew.m_vertices[i + 1], &csgNew.m_vertices[i + 2]);
        }
    }

    void CreateSubractiveBrushes() {
        //Timer timer("CreateSubractiveBrushes()");
        g_subtractiveBrushes.clear();
        // Subtractive cubes
        for (int i = 0; i < Scene::g_csgSubtractiveCubes.size(); i++) {
            CSGCube& csgShape = Scene::g_csgSubtractiveCubes[i];
            Transform transform = csgShape.GetTransform();
            transform.scale *= glm::vec3(0.5f);
            Brush& brush = g_subtractiveBrushes.emplace_back();
            brush.SetBrushType(AIR_BRUSH);
            brush.SetBrushShape(BrushShape::CUBE);
            brush.SetTransform(transform.to_mat4());
        }
        // Doors
        for (Door& door : Scene::GetDoors()) {
            Transform transform;
            transform.position = door.m_position + glm::vec3(0, DOOR_HEIGHT / 2, 0);
            transform.rotation = glm::vec3(0, door.m_rotation, 0);
            transform.scale = glm::vec3(0.2f, DOOR_HEIGHT, 0.81f);
            transform.scale *= glm::vec3(0.5f);
            Brush& brush = g_subtractiveBrushes.emplace_back();
            brush.SetBrushType(AIR_BRUSH);
            brush.SetBrushShape(BrushShape::CUBE);
            brush.SetTransform(transform.to_mat4());
        }
        // Windows
        for (Window& window : Scene::GetWindows()) {
            Transform transform;
            transform.position = window.GetPosition() + glm::vec3(0, 1.45f, 0);
            transform.rotation = glm::vec3(0, window.GetRotationY(), 0);
            transform.scale = glm::vec3(0.9f, 1.2f, 0.2f);
            transform.scale *= glm::vec3(0.5f);
            Brush& brush = g_subtractiveBrushes.emplace_back();
            brush.SetBrushType(AIR_BRUSH);
            brush.SetBrushShape(BrushShape::CUBE);
            brush.SetTransform(transform.to_mat4());
        }
    }

    void Build() {

        static int materialIndex = AssetManager::GetMaterialIndex("Ceiling2");

        std::cout << "\n";

        CleanUp();
        CreateSubractiveBrushes();

        g_csgNewObjects.clear();

        for (int i = 0; i < Scene::g_csgAdditiveWallPlanes.size(); i++) {
            CSGPlane& csgPlane = Scene::g_csgAdditiveWallPlanes[i];
            Brush brush;
            brush.SetBrushType(SOLID_BRUSH);
            brush.SetBrushShape(BrushShape::PLANE);
            brush.SetTransform(csgPlane.GetCSGMatrix());
            CreateCSGObjectFromBrush(brush, CSGObjecType::WALL_PLANE, i, csgPlane.materialIndex, csgPlane.textureScale, csgPlane.textureOffsetX, csgPlane.textureOffsetY);
        }
        for (int i = 0; i < Scene::g_csgAdditiveFloorPlanes.size(); i++) {
            CSGPlane& csgPlane = Scene::g_csgAdditiveFloorPlanes[i];
            Brush brush;
            brush.SetBrushType(SOLID_BRUSH);
            brush.SetBrushShape(BrushShape::PLANE);
            brush.SetTransform(csgPlane.GetCSGMatrix());
            CreateCSGObjectFromBrush(brush, CSGObjecType::FLOOR_PLANE, i, csgPlane.materialIndex, csgPlane.textureScale, csgPlane.textureOffsetX, csgPlane.textureOffsetY);
        }
        for (int i = 0; i < Scene::g_csgAdditiveCeilingPlanes.size(); i++) {
            CSGPlane& csgPlane = Scene::g_csgAdditiveCeilingPlanes[i];
            Brush brush;
            brush.SetBrushType(SOLID_BRUSH);
            brush.SetBrushShape(BrushShape::PLANE);
            brush.SetTransform(csgPlane.GetCSGMatrix());
            CreateCSGObjectFromBrush(brush, CSGObjecType::CEILING_PLANE, i, csgPlane.materialIndex, csgPlane.textureScale, csgPlane.textureOffsetX, csgPlane.textureOffsetY);
        }
        for (int i = 0; i < Scene::g_csgAdditiveCubes.size(); i++) {
            CSGCube& csgCube = Scene::g_csgAdditiveCubes[i];
            Brush brush;
            brush.SetBrushType(SOLID_BRUSH);
            brush.SetBrushShape(BrushShape::CUBE);
            brush.SetTransform(csgCube.GetCSGMatrix());
            CreateCSGObjectFromBrush(brush, CSGObjecType::CUBE, i, csgCube.materialIndex, csgCube.textureScale, csgCube.textureOffsetX, csgCube.textureOffsetY);
        }
        
        g_vertices.clear();
        g_indices.clear();

        // Hack in the door vertices. They need to be in this vector for raytracing.

        uint32_t modelIndex = AssetManager::GetModelIndexByName("Door");
        Model* model = AssetManager::GetModelByIndex(modelIndex);
        for (auto& meshIndex : model->GetMeshIndices()) {
            Mesh* mesh = AssetManager::GetMeshByIndex(meshIndex);
            if (mesh->name == "SM_Door") {
                CSGVertex v0, v1, v2, v3, v4, v5, v6, v7;
                v0.position = glm::vec3(mesh->aabbMin.x, mesh->aabbMin.y, mesh->aabbMin.z);
                v1.position = glm::vec3(mesh->aabbMin.x, mesh->aabbMax.y, mesh->aabbMin.z);
                v2.position = glm::vec3(mesh->aabbMin.x, mesh->aabbMax.y, mesh->aabbMax.z);
                v3.position = glm::vec3(mesh->aabbMin.x, mesh->aabbMin.y, mesh->aabbMax.z);
                v4.position = glm::vec3(mesh->aabbMax.x, mesh->aabbMin.y, mesh->aabbMax.z);
                v5.position = glm::vec3(mesh->aabbMax.x, mesh->aabbMax.y, mesh->aabbMax.z);
                v6.position = glm::vec3(mesh->aabbMax.x, mesh->aabbMax.y, mesh->aabbMin.z);
                v7.position = glm::vec3(mesh->aabbMax.x, mesh->aabbMin.y, mesh->aabbMin.z);
                g_vertices.push_back(v0);
                g_vertices.push_back(v1);
                g_vertices.push_back(v2);
                g_vertices.push_back(v3);
                g_vertices.push_back(v4);
                g_vertices.push_back(v5);
                g_vertices.push_back(v6);
                g_vertices.push_back(v7);
                g_indices.push_back(2);
                g_indices.push_back(1);
                g_indices.push_back(0);
                g_indices.push_back(0);
                g_indices.push_back(3);
                g_indices.push_back(2);
                g_indices.push_back(2 + 4);
                g_indices.push_back(1 + 4);
                g_indices.push_back(0 + 4);
                g_indices.push_back(0 + 4);
                g_indices.push_back(3 + 4);
                g_indices.push_back(2 + 4);
            }
        }
        g_baseCSGVertex = g_vertices.size();
        int baseIndex = g_indices.size();
        int baseVertex = g_vertices.size();

        g_objects.clear();


        for (CSGNew& csgNew : g_csgNewObjects) {

            CSGObject& csgObject = g_objects.emplace_back();
            csgObject.m_materialIndex = csgNew.m_materialIndex;
            csgObject.m_parentIndex = csgNew.m_parentIndex;
            if (csgNew.m_type == CSGObjecType::CUBE) {
                csgObject.m_parentObjectType = ObjectType::CSG_OBJECT_ADDITIVE_CUBE;
            }
            else if (csgNew.m_type == CSGObjecType::FLOOR_PLANE) {
                csgObject.m_parentObjectType = ObjectType::CSG_OBJECT_ADDITIVE_FLOOR_PLANE;
            }
            else if (csgNew.m_type == CSGObjecType::WALL_PLANE) {
                csgObject.m_parentObjectType = ObjectType::CSG_OBJECT_ADDITIVE_WALL_PLANE;
            }
            else if (csgNew.m_type == CSGObjecType::CEILING_PLANE) {
                csgObject.m_parentObjectType = ObjectType::CSG_OBJECT_ADDITIVE_CEILING_PLANE;
            }     

            int vertexCount = csgNew.m_vertices.size();
            int indexCount = csgNew.m_indices.size();

            g_vertices.insert(g_vertices.end(), csgNew.m_vertices.begin(), csgNew.m_vertices.end());
            g_indices.insert(g_indices.end(), csgNew.m_indices.begin(), csgNew.m_indices.end());

            csgObject.m_baseIndex = baseIndex;
            csgObject.m_baseVertex = baseVertex;
            csgObject.m_vertexCount = vertexCount;
            csgObject.m_indexCount = indexCount;
            csgObject.m_aabb = csgNew.m_aabb;

            baseVertex += vertexCount;
            baseIndex += vertexCount;
        }

        {
            //Timer timer4("-creating csg object physx objects");
            for (CSGObject& csgObject : g_objects) {
                if (csgObject.m_vertexCount > 0) {
                    csgObject.CreatePhysicsObjectFromVertices();
                }
            }
        }



        // Nav mesh vertices
        g_navMeshVertices.reserve(g_vertices.size());
        for (CSGObject& csgObject : g_objects) {
            for (int i = csgObject.m_baseVertex; i < csgObject.m_baseVertex + csgObject.m_vertexCount; i += 3) {
                g_navMeshVertices.push_back(g_vertices[i + 0].position);
                g_navMeshVertices.push_back(g_vertices[i + 1].position);
                g_navMeshVertices.push_back(g_vertices[i + 2].position);
            }
        }


        {
            //Timer timer4("-uploading csg geo to gpu");
            g_sceneDirty = true;
            if (g_sceneDirty && g_vertices.size()) {
                if (BackEnd::GetAPI() == API::OPENGL) {
                    OpenGLBackEnd::UploadConstructiveSolidGeometry(g_vertices, g_indices);
                }
                else if (BackEnd::GetAPI() == API::VULKAN) {

                }
                g_sceneDirty = false;
            }
            for (Light& light : Scene::g_lights) {
                light.m_shadowMapIsDirty = true;
            }
        }


        {
            GlobalIllumination::RecalculateGI();
        }
    }



    

    std::span<CSGVertex> CSG::GetRangedVerticesSpan(uint32_t baseVertex, uint32_t vertexCount) {
        return std::span<CSGVertex>(CSG::g_vertices.data() + baseVertex, vertexCount);
    }

    std::span<uint32_t> CSG::GetRangedIndicesSpan(uint32_t baseIndex, uint32_t indexCount) {
        return std::span<uint32_t>(CSG::g_indices.data() + baseIndex, indexCount);
    }

    std::vector<CSGVertex>& GetVertices() {
        return g_vertices;
    }

    std::vector<uint32_t>& GetIndices() {
        return g_indices;
    }

    std::vector<glm::vec3>& GetNavMeshVertices() {
        return g_navMeshVertices;
    }

    std::vector<CSGObject>& GetCSGObjects() {
        return g_objects;
    }

    uint32_t GetBaseCSGVertex() {
        return g_baseCSGVertex;
    }

    void SetDirtyFlag(bool value) {
        g_sceneDirty = value;
    }

    bool  IsDirty() {
        return g_sceneDirty;
    }

    void UpdateDisplayLists() {
        std::vector<std::future<void>> futures;
        for (CSGObject& csgObject : g_objects) {
            futures.push_back(std::async(std::launch::async, [&csgObject]() {
                // Update
                csg::brush_t* b = csgObject.m_brush;
                Brush* brush = any_cast<Brush>(&b->userdata);
                brush->update_display_list();
            }));
        }
        for (auto& future : futures) {
            future.get();
        }
    }
}

std::span<CSGVertex> CSGObject::GetVerticesSpan() {
    return std::span<CSGVertex>(CSG::g_vertices.data() + m_baseVertex, m_vertexCount);
}

std::span<uint32_t> CSGObject::GetIndicesSpan() {
    return std::span<uint32_t>(CSG::g_indices.data() + m_baseIndex, m_indexCount);
}

void CSGObject::CleanUpPhysicsObjects() {
    Physics::Destroy(m_triangleMesh);
    Physics::Destroy(m_pxRigidStatic);
    Physics::Destroy(m_pxShape);
}

void CSGObject::CreatePhysicsObjectFromVertices() {

    CleanUpPhysicsObjects();

    if (m_vertexCount > 0) {

        // Create triangle mesh
        int count = GetVerticesSpan().size();
        std::vector<PxVec3> pxvertices;
        std::vector<unsigned int> pxindices;
        pxvertices.reserve(count);
        pxindices.reserve(count);
        for (CSGVertex& vertex : GetVerticesSpan()) {
            pxvertices.push_back(PxVec3(vertex.position.x, vertex.position.y, vertex.position.z));
        }
        for (int i = 0; i < m_vertexCount; i++) {
            pxindices.push_back(i);
        }

        m_triangleMesh = Physics::CreateTriangleMesh(pxvertices.size(), pxvertices.data(), pxindices.size() / 3, pxindices.data());

        if (!m_triangleMesh) {
            std::cout << "pxvertices.size(): " << pxvertices.size() << "\n";
            std::cout << "pxindices.size(): " << pxindices.size() << "\n";
            std::cout << "CSGObject::CreatePhysicsObjectFromVertices() failed, m_triangleMesh failed to create\n!";
        }



        if (m_triangleMesh || true) {
            PhysicsFilterData filterData;
            filterData.raycastGroup = RAYCAST_ENABLED;
            filterData.collisionGroup = ENVIROMENT_OBSTACLE;
            filterData.collidesWith = (CollisionGroup)(GENERIC_BOUNCEABLE | BULLET_CASING | PLAYER | RAGDOLL);
            PxShapeFlags shapeFlags(PxShapeFlag::eSCENE_QUERY_SHAPE | PxShapeFlag::eSIMULATION_SHAPE);

            m_pxShape = Physics::CreateShapeFromTriangleMesh(m_triangleMesh, shapeFlags);
            m_pxRigidStatic = Physics::CreateRigidStatic(Transform(), filterData, m_pxShape);
            m_pxRigidStatic->userData = new PhysicsObjectData(m_parentObjectType, this);
        }
        else {
        //    std::cout << "CSGObject::CreatePhysicsObjectFromVertices() failed, m_triangleMesh failed to create\n!";
        }
    
    }
}
