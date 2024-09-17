#include "csg.h"
#include "brush.h"
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
    csg::world_t g_world;
    std::vector<CSGVertex> g_vertices;
    std::vector<uint32_t> g_indices;
    bool g_sceneDirty = true;
    uint32_t g_baseCSGVertex = 0;
    std::vector<glm::vec3> g_navMeshVertices;

    void UpdateDisplayLists();

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
        g_navMeshVertices.clear();
    }

    void Build() {

        std::cout << "\n";

        CleanUp();

        {
            Timer timer2("-brush stuff");

            // Remove all brushes
            csg::brush_t* brush = g_world.first();
            while (brush) {
                g_world.remove(brush);
                brush = g_world.first();
            }

            static int materialIndex = AssetManager::GetMaterialIndex("Ceiling2");

            for (Staircase& staircase : Scene::g_staircases) {

                float offsetY = 0;
                float offsetZ = 0;

                for (int i = 0; i < staircase.m_stepCount; i++) {

                    Transform stepTransform;
                    stepTransform.position.y = offsetY;
                    stepTransform.position.z = offsetZ;
                    stepTransform.position.y += (0.122f * 0.5f);
                    stepTransform.position.z += (0.122f * 0.5f);
                    stepTransform.scale.y = 0.122f;
                    stepTransform.scale.z = 0.122f;

                    Transform worldTransform;
                    worldTransform.position = Util::GetTranslationFromMatrix(staircase.GetModelMatrix() * stepTransform.to_mat4());
                    worldTransform.rotation.y = staircase.m_rotation;
                    worldTransform.scale.y = 0.122f;
                    worldTransform.scale.z = 0.122f;

                    CSGObject& csg = g_objects.emplace_back();
                    csg.m_transform = worldTransform;
                    csg.m_type = CSGType::ADDITIVE_CUBE;
                    csg.m_materialIndex = materialIndex;
                    csg.m_textureScale = 1;
                    csg.m_parentIndex = i;
                    csg.m_textureOffsetX = 1;
                    csg.m_textureOffsetY = 1;
                    csg.m_disableRendering = true;

                    float magic = 0.202 - 0.052;
                    magic = 0.432 * 0.333333f;
                    offsetY += magic;
                    offsetZ += magic;
                }
            }



            for (int i = 0; i < Scene::g_csgAdditiveShapes.size(); i++) {
                CSGShape& csgShape = Scene::g_csgAdditiveShapes[i];
                CSGObject& csgObject = g_objects.emplace_back();
                csgObject.m_transform = csgShape.GetTransform();
                if (csgShape.m_brushShape == BrushShape::CUBE) {
                    csgObject.m_type = CSGType::ADDITIVE_CUBE;
                }
                else if (csgShape.m_brushShape == BrushShape::PLANE) {
                    csgObject.m_type = CSGType::ADDITIVE_PLANE;
                }
                csgObject.m_materialIndex = csgShape.materialIndex;
                csgObject.m_textureScale = csgShape.textureScale;
                csgObject.m_parentIndex = i;
                csgObject.m_textureOffsetX = csgShape.textureOffsetX;
                csgObject.m_textureOffsetY = csgShape.textureOffsetY;
                csgObject.m_parentVolumeNormalMatrix = csgShape.GetNormalMatrix();
            }

            for (int i = 0; i < Scene::g_csgSubtractiveShapes.size(); i++) {
                CSGShape& csgShape = Scene::g_csgSubtractiveShapes[i];
                CSGObject& csg = g_objects.emplace_back();
                csg.m_transform = csgShape.GetTransform();
                csg.m_type = CSGType::SUBTRACTIVE;
                csg.m_materialIndex = csgShape.materialIndex;
                csg.m_textureScale = csgShape.textureScale;
                csg.m_parentIndex = i;
                csg.m_textureOffsetX = csgShape.textureOffsetX;
                csg.m_textureOffsetY = csgShape.textureOffsetY;
                csg.m_parentVolumeNormalMatrix = csgShape.GetNormalMatrix();
            }

            for (Door& door : Scene::GetDoors()) {
                CSGObject& csg = g_objects.emplace_back();
                csg.m_transform.position = door.m_position + glm::vec3(0, DOOR_HEIGHT / 2, 0);
                csg.m_transform.rotation = glm::vec3(0, door.m_rotation, 0);
                csg.m_transform.scale = glm::vec3(0.2f, DOOR_HEIGHT, 0.8f);
                csg.m_type = CSGType::DOOR;
                csg.m_materialIndex = AssetManager::GetMaterialIndex("FloorBoards"); // add this to the door object somehow
                csg.m_textureScale = 0.5f;                                           // add this to the door object somehow
            }

            for (Window& window : Scene::GetWindows()) {
                CSGObject& csg = g_objects.emplace_back();
                csg.m_transform.position = window.GetPosition() + glm::vec3(0, 1.5f, 0);
                csg.m_transform.scale = glm::vec3(0.8f, 1.3f, 0.2f);
                csg.m_transform.rotation.y = window.GetRotationY();
                csg.m_type = CSGType::SUBTRACTIVE;
                csg.m_materialIndex = AssetManager::GetMaterialIndex("FloorBoards"); // add this to the door object somehow
                csg.m_textureScale = 0.5f;                                           // add this to the door object somehow
            }


            for (int i = 0; i < g_objects.size(); i++) {

                CSGObject& csgObject = g_objects[i];
                csgObject.m_brush = g_world.add();
                csgObject.m_brush->userdata = Brush(csgObject.m_brush);
                Brush* userdata = any_cast<Brush>(&csgObject.m_brush->userdata);

                Transform transform = csgObject.m_transform;
                transform.scale *= glm::vec3(0.5f);

                if (csgObject.m_type == CSGType::ADDITIVE_CUBE) {
                    userdata->SetBrushType(SOLID_BRUSH);
                    userdata->SetBrushShape(BrushShape::CUBE);
                }
                else if (csgObject.m_type == CSGType::ADDITIVE_PLANE) {
                    userdata->SetBrushType(SOLID_BRUSH);
                    userdata->SetBrushShape(BrushShape::PLANE);
                    transform.scale *= glm::vec3(1.0f, 1.0f, 0.00001f);
                }
                else if (csgObject.m_type == CSGType::SUBTRACTIVE) {
                    userdata->SetBrushType(AIR_BRUSH);
                }
                else if (csgObject.m_type == CSGType::DOOR) {
                    userdata->SetBrushType(AIR_BRUSH);
                }
                else if (csgObject.m_type == CSGType::WINDOW) {
                    userdata->SetBrushType(AIR_BRUSH);
                }
                userdata->SetTransform(transform.to_mat4());
            }

            // Calculate vertices

        }
        {
            Timer timer3("-g_world.rebuild()");
            g_world.rebuild();
        }


        {
            Timer timer3("-vertex stuff");

            g_vertices.clear();
            g_indices.clear();

            // Hack in the door vertices. They need to be in this vector for raytracing.

            uint32_t modelIndex = AssetManager::GetModelIndexByName("Door");
            Model* model = AssetManager::GetModelByIndex(modelIndex);
            for (auto& meshIndex : model->GetMeshIndices()) {
                Mesh* mesh = AssetManager::GetMeshByIndex(meshIndex);
                if (mesh->name == "SM_Door") {
                    //std::cout << Util::Vec3ToString10(mesh->aabbMin) << "\n";
                    //std::cout << Util::Vec3ToString10(mesh->aabbMax) << "\n";
                    //(-0.00, 0.00, 0.00)
                    //(-0.00, 2.00, 0.79)
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

            // Do some important CSG thing
            UpdateDisplayLists();

            // Reserve enough space in g_vertices
            int vertexCount = baseVertex;
            for (CSGObject& csgObject : g_objects) {
                csg::brush_t* b = csgObject.m_brush;
                Brush* brush = any_cast<Brush>(&b->userdata);
                vertexCount += brush->m_vertices.size();
            }
            g_vertices.reserve(vertexCount);


            // Inflate the triangles
            float inflateAmount = 0.0001f;
            std::vector<std::future<void>> futures;
            for (CSGObject& csgObject : g_objects) {
                // Access the brush object
                csg::brush_t* b = csgObject.m_brush;
                Brush* brush = any_cast<Brush>(&b->userdata);
                // Capture brush and inflateAmount in the lambda
                futures.push_back(std::async(std::launch::async, [brush, inflateAmount]() {
                    for (size_t i = 0; i < brush->m_vertices.size(); i += 3) {
                        glm::vec3& v0 = brush->m_vertices[i + 0].position;
                        glm::vec3& v1 = brush->m_vertices[i + 1].position;
                        glm::vec3& v2 = brush->m_vertices[i + 2].position;
                        glm::vec3 center = (v0 + v1 + v2) / 3.0f;
                        // Inflate each vertex based on its position relative to the center
                        auto InflateVertex = [&](glm::vec3& vertex) {
                            if (vertex.x < center.x) {
                                vertex.x -= inflateAmount;
                            }
                            else if (vertex.x > center.x) {
                                vertex.x += inflateAmount;
                            }
                            if (vertex.y < center.y) {
                                vertex.y -= inflateAmount;
                            }
                            else if (vertex.y > center.y) {
                                vertex.y += inflateAmount;
                            }
                            if (vertex.z < center.z) {
                                vertex.z -= inflateAmount;
                            }
                            else if (vertex.z > center.z) {
                                vertex.z += inflateAmount;
                            }
                        };
                        InflateVertex(v0);
                        InflateVertex(v1);
                        InflateVertex(v2);
                    }
                }));
            }
            // Wait for all threads to finish
            for (auto& future : futures) {
                future.get();
            }

            futures.clear();





            for (CSGObject& csgObject : g_objects) {

                glm::vec3 boundsMin = glm::vec3(1e30f);
                glm::vec3 boundsMax = glm::vec3(-1e30f);

                csg::brush_t* b = csgObject.m_brush;
                Brush* brush = any_cast<Brush>(&b->userdata);
                
                

                int vertexCount = 0;
                int indexCount = 0;
                int index = 0;

               // float inflateAmount = 0.1f;
                //glm::vec3 center = Util::GetTranslationFromMatrix(csgObject.m_transform.to_mat4());

                for (CSGVertex& vertex : brush->m_vertices) {

                  /*  glm::vec3 inflatedVertex = vertex.position;
                    if (vertex.position.x < center.x) {
                        inflatedVertex.x -= inflateAmount;
                    }
                    else if (vertex.position.x > center.x) {
                        inflatedVertex.x += inflateAmount;
                    }
                    if (vertex.position.y < center.y) {
                        inflatedVertex.y -= inflateAmount;
                    }
                    else if (vertex.position.y > center.y) {
                        inflatedVertex.y += inflateAmount;
                    }
                    if (vertex.position.z < center.z) {
                        inflatedVertex.z -= inflateAmount;
                    }
                    else if (vertex.position.z > center.z) {
                        inflatedVertex.z += inflateAmount;
                    }
                    vertex.position = inflatedVertex;
                    */











                    glm::vec3 origin = glm::vec3(0, 0, 0);
                    origin = glm::vec3(0);
                    vertex.uv = CalculateUV(vertex.position, vertex.normal, origin);
                    vertex.uv *= csgObject.m_textureScale;
                    vertex.uv.x += csgObject.m_textureOffsetX;
                    vertex.uv.y += csgObject.m_textureOffsetY;

                    g_vertices.push_back(vertex);
                    vertexCount++; // remove duplicate vertices somehow ya fool
                    indexCount++;

                    boundsMin = Util::Vec3Min(boundsMin, vertex.position);
                    boundsMax = Util::Vec3Max(boundsMax, vertex.position);

                    g_indices.push_back(index);
                    index++;
                }
                csgObject.m_baseIndex = baseIndex;
                csgObject.m_baseVertex = baseVertex;
                csgObject.m_vertexCount = vertexCount;
                csgObject.m_indexCount = indexCount;
                csgObject.m_aabb = AABB(boundsMin, boundsMax);

                //std::cout << baseVertex << "\n";
                //std::cout << " -boundsMin:" << Util::Vec3ToString(boundsMin) << " " << Util::Vec3ToString(b->box.min) << "\n";
                //std::cout << " -boundsMax:" << Util::Vec3ToString(boundsMax) << " " << Util::Vec3ToString(b->box.max) << "\n\n";

                baseVertex += vertexCount;
                baseIndex += vertexCount;
            }




            // Calculate tangents
            for (int i = g_baseCSGVertex; i < g_vertices.size(); i += 3) {
                Util::SetNormalsAndTangentsFromVertices(&g_vertices[i], &g_vertices[i + 1], &g_vertices[i + 2]);
            }

            // Set materials based on local face normal
            g_navMeshVertices.reserve(g_vertices.size());

            for (CSGObject& csgObject : g_objects) {
                //glm::mat4 inverseNormalMatrix = glm::inverse(csgObject.m_parentVolumeNormalMatrix);
                for (int i = csgObject.m_baseVertex; i < csgObject.m_baseVertex + csgObject.m_vertexCount; i += 3) {

                    /*glm::vec4 transformedNormal = inverseNormalMatrix * glm::vec4(g_vertices[i].normal, 0.0f);
                    glm::vec3 transformedNormalXYZ = glm::normalize(glm::vec3(transformedNormal));

                    if (Util::AreNormalsAligned(transformedNormalXYZ, glm::vec3(0, 0, -1), 0.9f)) {
                        g_vertices[i + 0].materialIndex = AssetManager::GetMaterialIndex("Ceiling2");
                        g_vertices[i + 1].materialIndex = AssetManager::GetMaterialIndex("Ceiling2");;
                        g_vertices[i + 2].materialIndex = AssetManager::GetMaterialIndex("Ceiling2");
                    }*/

                    // Nav mesh vertices
                    //float dotProduct = glm::dot(g_vertices[i].normal, glm::vec3(0,1,0));
                    //float minCosAngle = glm::cos(glm::radians(45.0f));
                    //if (dotProduct >= minCosAngle && dotProduct <= 1.0f) {
                    g_navMeshVertices.push_back(g_vertices[i + 0].position);
                    g_navMeshVertices.push_back(g_vertices[i + 1].position);
                    g_navMeshVertices.push_back(g_vertices[i + 2].position);
                    //}
                }
            }
        }

        {
            Timer timer4("-creating csg object physx objects");
            for (CSGObject& cube : g_objects) {
                cube.CreatePhysicsObjectFromVertices();
            }
        }


        {
            Timer timer4("-uploading csg geo to gpu");
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
                light.isDirty = true;
            }
        }


        {
            GlobalIllumination::RecalculateAll();
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

    if (m_type == CSGType::ADDITIVE_CUBE || m_type == CSGType::ADDITIVE_PLANE && m_vertexCount > 0) {

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
