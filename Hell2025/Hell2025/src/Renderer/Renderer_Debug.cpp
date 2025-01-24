#include "Renderer.h"
#include "../Physics/Physics.h"
#include "../Editor/Editor.h"
#include "../Game/Game.h"
#include "../Game/Scene.h"
#include "../Enemies/Shark/SharkPathManager.h"
#include "../Renderer/GlobalIllumination.h"
#include "../Pathfinding/Pathfinding2.h"
#include "../Renderer/Raytracing/Raytracing.h"
#include "../Renderer/RendererData.h"
#include "../Renderer/RendererUtil.hpp"
#include "../Math/Frustum.h"

#include "../Math/BVH.h"
#include "tinycsg/tinycsg.hpp"

#include "RapidHotload.h"

/*
 █▀█ █▀█ ▀█▀ █▀█ ▀█▀ █▀▀
 █▀▀ █ █  █  █ █  █  ▀▀█
 ▀   ▀▀▀ ▀▀▀ ▀ ▀  ▀  ▀▀▀ */

std::string g_debugText = "";

AABB g_testAABB = {
    glm::vec3(-0.75f, 0.75f, -0.25f),
    glm::vec3(-0.25f, 1.25f, 0.25f)
};

Sphere g_testSphere = {
    glm::vec3(0.5f, 1.0f, 0.0f),
    0.35f
};

void Renderer::UpdateDebugPointsMesh() {

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    if (Editor::IsOpen() && Game::g_editorMode == EditorMode::SHARK_PATH) {
        // Draw WIP Shark Path
        if (!Editor::g_sharkPath.empty()) {
            for (int i = 0; i < Editor::g_sharkPath.size(); i++) {
                vertices.push_back({ Editor::g_sharkPath[i], YELLOW });
            }
        }
    }
    // Draw all existing shark paths
    Shark& shark = Scene::GetShark();
    if (shark.m_drawPath) {
        for (SharkPath& sharkPath : SharkPathManager::GetSharkPaths()) {
            for (int i = 0; i < sharkPath.m_points.size(); i++) {
                vertices.push_back({ sharkPath.m_points[i].position, WHITE });
            }
        }
    }

    // Shark points
    std::vector<Vertex> sharkVertices = shark.GetDebugPointVertices();
    vertices.insert(std::end(vertices), std::begin(sharkVertices), std::end(sharkVertices));


    Player* player = Game::GetPlayerByIndex(0);

    if (g_debugLineRenderMode == DebugLineRenderMode::PATHFINDING_RECAST) {
        // Dobermann path
        for (Dobermann& dobermann : Scene::g_dobermann) {
            for (int i = 0; i < dobermann.m_pathToTarget.points.size(); i++) {
                vertices.push_back(Vertex(dobermann.m_pathToTarget.points[i], RED));
            }
        }
    }

    // Plane vertices
    if (Editor::GetSelectedObjectIndex() != -1) {
        CSGPlane* csgPlane = nullptr;
        if (Editor::GetSelectedObjectType() == ObjectType::CSG_OBJECT_ADDITIVE_WALL_PLANE) {
            csgPlane = Scene::GetWallPlaneByIndex(Editor::GetSelectedObjectIndex());
        }
        if (Editor::GetSelectedObjectType() == ObjectType::CSG_OBJECT_ADDITIVE_FLOOR_PLANE) {
            csgPlane = Scene::GetFloorPlaneByIndex(Editor::GetSelectedObjectIndex());
        }
        if (Editor::GetSelectedObjectType() == ObjectType::CSG_OBJECT_ADDITIVE_CEILING_PLANE) {
            CSGObject& csgObject = CSG::GetCSGObjects()[Editor::GetSelectedObjectIndex()];
            csgPlane = Scene::GetCeilingPlaneByIndex(csgObject.m_parentIndex);
        }
        if (csgPlane) {
            vertices.push_back(Vertex(csgPlane->m_veritces[0], ORANGE));
            vertices.push_back(Vertex(csgPlane->m_veritces[1], ORANGE));
            vertices.push_back(Vertex(csgPlane->m_veritces[2], ORANGE));
            vertices.push_back(Vertex(csgPlane->m_veritces[3], ORANGE));
        }
    }
    // BROKEN BELOW
    // BROKEN BELOW
    // BROKEN BELOW
    // BROKEN BELOW
    // BROKEN BELOW
    // BROKEN BELOW
    // BROKEN BELOW
    // BROKEN BELOW
    //if (Editor::GetSelectedObjectType() == ObjectType::CSG_OBJECT_ADDITIVE_FLOOR_PLANE) {
    //    CSGPlane& csgPlane = Scene::g_csgAdditiveFloorPlanes[Editor::GetSelectedObjectIndex()];
    //    vertices.push_back(Vertex(csgPlane.m_veritces[0], ORANGE));
    //    vertices.push_back(Vertex(csgPlane.m_veritces[1], ORANGE));
    //    vertices.push_back(Vertex(csgPlane.m_veritces[2], ORANGE));
    //    vertices.push_back(Vertex(csgPlane.m_veritces[3], ORANGE));
    //}
    //if (Editor::GetSelectedObjectType() == ObjectType::CSG_OBJECT_ADDITIVE_CEILING_PLANE) {
    //    CSGPlane& csgPlane = Scene::g_csgAdditiveCeilingPlanes[Editor::GetSelectedObjectIndex()];
    //    vertices.push_back(Vertex(csgPlane.m_veritces[0], ORANGE));
    //    vertices.push_back(Vertex(csgPlane.m_veritces[1], ORANGE));
    //    vertices.push_back(Vertex(csgPlane.m_veritces[2], ORANGE));
    //    vertices.push_back(Vertex(csgPlane.m_veritces[3], ORANGE));
    //}


    // BROKEN ABOVE
    // BROKEN ABOVE
    // BROKEN ABOVE
    // BROKEN ABOVE
    // BROKEN ABOVE
    // BROKEN ABOVE
    // BROKEN ABOVE
    // BROKEN ABOVE
    if (Editor::GetHoveredVertexIndex() != -1) {
    //    vertices.push_back(Vertex(Editor::GetHoveredVertexPosition(), WHITE));
    }
    if (Editor::GetSelectedVertexIndex() != -1) {
     //   vertices.push_back(Vertex(Editor::GetSelectedVertexPosition(), WHITE));
    }


    /*
    HeightMap& heightMap = AssetManager::g_heightMap;
    TreeMap& treeMap = AssetManager::g_treeMap;

    float offsetX = heightMap.m_width * heightMap.m_transform.scale.x * -0.5f;
    float offsetZ = heightMap.m_depth * heightMap.m_transform.scale.z * -0.5f;

    std::cout << "\offsetX: " << offsetX << "\n";
    std::cout << "offsetZ: " << offsetZ << "\n";

    static std::vector<Vertex> treeMapVertices;
    if (treeMapVertices.empty()) {
        for (int x = 0; x < treeMap.m_width; x++) {
            for (int z = 0; z < treeMap.m_depth; z++) {
                if (treeMap.m_array[x][z] == 1) {
                    Vertex& vertex = treeMapVertices.emplace_back();
                    vertex.position.x = x * heightMap.m_transform.scale.x + offsetX;
                    vertex.position.z = z * heightMap.m_transform.scale.z + offsetZ;
                    vertex.normal = RED;
                }
            }
        }
    }


    vertices = treeMapVertices;*/



    /*
    if (true) {
        static std::vector<Vertex> g_saplings;
        if (g_saplings.empty()) {
            int iterationMax = 10000;
            int desiredSapplingCount = 500;
            TreeMap& treeMap = AssetManager::g_treeMap;
            g_saplings.reserve(desiredSapplingCount);
            for (int i = 0; i < iterationMax; i++) {
                float x = Util::RandomFloat(0, treeMap.m_width);
                float z = Util::RandomFloat(0, treeMap.m_depth);

                if (treeMap.m_array[x][z] == 1) {
                    Vertex& vertex = g_saplings.emplace_back();
                    vertex.position.x = x * heightMap.m_transform.scale.x + offsetX;
                    vertex.position.y = 0.1f;
                    vertex.position.z = z * heightMap.m_transform.scale.z + offsetZ;
                    vertex.normal = WHITE;
                    std::cout << x << ", " << z << " ACCE\n";
                }
                else {
                    std::cout << x << ", " << z << " was rejected\n";
                }
                if (g_saplings.size() == desiredSapplingCount) {
                    break;
                }
            }

        }
        vertices.reserve(vertices.size() + g_saplings.size());
        vertices.insert(std::end(vertices), std::begin(g_saplings), std::end(g_saplings));
    }*/




    /*
    for (int i = 0; i < heightMap.m_vertices.size(); i++) {
        Vertex& vertex = heightMap.m_vertices[i];

        glm::vec3 position = heightMap.m_transform.to_mat4() * glm::vec4(vertex.position, 1.0);
        vertices.push_back(Vertex(position, WHITE));
    }
    */
    //float minX = heightMap.m_transform.position.x * heightMap.m_vertexScale * heightMap.m_transform.scale.x;
    //float minY = -1.75;
    //float minZ = heightMap.m_transform.position.z * heightMap.m_vertexScale * heightMap.m_transform.scale.z;
   // glm::vec3 corner0 = glm::vec4(minX, minY, minZ, 1.0);
   // vertices.push_back(Vertex(corner0, WHITE));


    for (int i = 0; i < vertices.size(); i++) {
        indices.push_back(i);
    }
    g_debugPointsMesh.UpdateVertexBuffer(vertices, indices);
}

/*
 █   ▀█▀ █▀█ █▀▀ █▀▀
 █    █  █ █ █▀▀ ▀▀█
 ▀▀▀ ▀▀▀ ▀ ▀ ▀▀▀ ▀▀▀ */

glm::vec3 PixelToNDC(int pixelX, int pixelY, int viewportWidth, int viewportHeight) {
    float ndcX = (2.0f * pixelX) / viewportWidth - 1.0f;
    float ndcY = 1.0f - (2.0f * pixelY) / viewportHeight;  // Flip y-axis to match OpenGL's coordinate system
    return glm::vec3(ndcX, ndcY, 0.0f);
}

void Renderer::UpdateDebugLinesMesh() {

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;


    if (Editor::IsOpen() && Game::g_editorMode == EditorMode::SHARK_PATH) {
        // Draw WIP Shark Path
        if (!Editor::g_sharkPath.empty()) {
            for (int i = 0; i < Editor::g_sharkPath.size() - 1; i++) {
                vertices.push_back({ Editor::g_sharkPath[i], RED });
                vertices.push_back({ Editor::g_sharkPath[i + 1], RED });
            }
        }
    }
    // Draw all existing shark paths
    Shark& shark = Scene::GetShark();
    if (shark.m_drawPath) {
        for (SharkPath& sharkPath : SharkPathManager::GetSharkPaths()) {
            for (int i = 0; i < sharkPath.m_points.size() - 1; i++) {
                vertices.push_back({ sharkPath.m_points[i].position, WHITE });
                vertices.push_back({ sharkPath.m_points[i + 1].position, WHITE });

                // Draw it's direction as two lines (like an arrow)
                glm::vec3 position = sharkPath.m_points[i].position;
                glm::vec3 forward = -sharkPath.m_points[i].forward;
                glm::vec3 left = sharkPath.m_points[i].left;
                glm::vec3 right = sharkPath.m_points[i].right;
                glm::vec leftArrowPosition = glm::mix(position + forward, position + left, 0.5f);
                glm::vec rightArrowPosition = glm::mix(position + forward, position + right, 0.5f);

                vertices.push_back({ position , WHITE });
                vertices.push_back({ leftArrowPosition , WHITE });
                vertices.push_back({ position , WHITE });
                vertices.push_back({ rightArrowPosition , WHITE });
            }
            vertices.push_back({ sharkPath.m_points[0].position, WHITE });
            vertices.push_back({ sharkPath.m_points[sharkPath.m_points.size() - 1].position, WHITE });


            // Draw it's direction as two lines (like an arrow)
            int finalPointIndex = sharkPath.m_points.size() - 1;
            glm::vec3 position = sharkPath.m_points[finalPointIndex].position;
            glm::vec3 forward = -sharkPath.m_points[finalPointIndex].forward;
            glm::vec3 left = sharkPath.m_points[finalPointIndex].left;
            glm::vec3 right = sharkPath.m_points[finalPointIndex].right;
            glm::vec leftArrowPosition = glm::mix(position + forward, position + left, 0.5f);
            glm::vec rightArrowPosition = glm::mix(position + forward, position + right, 0.5f);
        }
    }

    // Shark lines
    std::vector<Vertex> sharkVertices = shark.GetDebugLineVertices();
    vertices.insert(std::end(vertices), std::begin(sharkVertices), std::end(sharkVertices));

    std::vector<PxRigidActor*> ignoreList;

    // Skip debug lines for player 0 ragdoll
    Player* player = Game::GetPlayerByIndex(0);
    AnimatedGameObject* characterModel = Scene::GetAnimatedGameObjectByIndex(player->GetCharacterModelAnimatedGameObjectIndex());
    for (auto r : characterModel->m_ragdoll.m_rigidComponents) {
        ignoreList.push_back(r.pxRigidBody);
    }

 // for (Brush& brush : CSG::g_subtractiveBrushes) {        
 //     auto& brushVertices = brush.GetCubeTriangles();
 //     for (int i = 0; i < brushVertices.size(); i+=3) {
 //         vertices.push_back(Vertex(brushVertices[i]));
 //         vertices.push_back(Vertex(brushVertices[i + 1]));
 //
 //         vertices.push_back(Vertex(brushVertices[i + 1]));
 //         vertices.push_back(Vertex(brushVertices[i + 2]));
 //
 //         vertices.push_back(Vertex(brushVertices[i]));
 //         vertices.push_back(Vertex(brushVertices[i + 2]));
 //     }
 // }



    if (Editor::IsOpen() && false) {
        std::vector<CSGObject> cubes = CSG::GetCSGObjects();
        for (CSGObject& cube : cubes) {
            std::span<CSGVertex> cubeVertices = cube.GetVerticesSpan();
            for (int i = 0; i < cubeVertices.size(); i += 3) {
                Vertex v0, v1, v2;
                v0.position = cubeVertices[i + 0].position;
                v1.position = cubeVertices[i + 1].position;
                v2.position = cubeVertices[i + 2].position;
                v0.normal = GREEN;
                v1.normal = GREEN;
                v2.normal = GREEN;
                if (cube.m_type == CSGType::SUBTRACTIVE) {
                    v0.normal = RED;
                    v1.normal = RED;
                    v2.normal = RED;
                }
                vertices.push_back(v0);
                vertices.push_back(v1);
                vertices.push_back(v1);
                vertices.push_back(v2);
                vertices.push_back(v2);
                vertices.push_back(v0);
            }
        }
    }
    else {
        if (g_debugLineRenderMode == DebugLineRenderMode::PHYSX_ALL ||
            g_debugLineRenderMode == DebugLineRenderMode::PHYSX_COLLISION ||
            g_debugLineRenderMode == DebugLineRenderMode::PHYSX_RAYCAST ||
            g_debugLineRenderMode == DebugLineRenderMode::PHYSX_EDITOR) {
            std::vector<Vertex> physicsDebugLineVertices = Physics::GetDebugLineVertices(g_debugLineRenderMode, ignoreList);
            vertices.reserve(vertices.size() + physicsDebugLineVertices.size());
            vertices.insert(std::end(vertices), std::begin(physicsDebugLineVertices), std::end(physicsDebugLineVertices));
        }
        else if (g_debugLineRenderMode == DebugLineRenderMode::BOUNDING_BOXES) {
            /*for (GameObject & gameObject : Scene::GetGamesObjects()) {
                std::vector<Vertex> aabbVertices = gameObject.GetAABBVertices();
                vertices.reserve(vertices.size() + aabbVertices.size());
                vertices.insert(std::end(vertices), std::begin(aabbVertices), std::end(aabbVertices));
            }*/
            for (auto& renderItem : RendererData::g_sceneGeometryRenderItems) {
                AABB aabb(renderItem.aabbMin, renderItem.aabbMax);
                DrawAABB(aabb, RED);
            }
        }

        else if (g_debugLineRenderMode == DebugLineRenderMode::RTX_LAND_AABBS) {
            for (Door& door : Scene::GetDoors()) {
                std::vector<Vertex> aabbVertices = Util::GetAABBVertices(door._aabb, GREEN);
                vertices.reserve(vertices.size() + aabbVertices.size());
                vertices.insert(std::end(vertices), std::begin(aabbVertices), std::end(aabbVertices));
            }
        }
        else if (g_debugLineRenderMode == DebugLineRenderMode::RTX_LAND_BOTTOM_LEVEL_ACCELERATION_STRUCTURES) {
            for (CSGObject& csgObject : CSG::GetCSGObjects()) {
                BLAS* blas = Raytracing::GetBLASByIndex(csgObject.m_blasIndex);
                if (blas) {
                    for (BVHNode& node : blas->bvhNodes) {
                        if (node.IsLeaf()) {
                            AABB aabb(node.aabbMin, node.aabbMax);
                            //DrawAABB(aabb, RED);
                        }
                    }
                }
            }
            TLAS* tlas = Raytracing::GetTLASByIndex(0);
            if (tlas) {
                for (int i = 0; i < tlas->GetInstanceCount(); i++) {
                    glm::mat4 worldTransform = tlas->GetInstanceWorldTransformByInstanceIndex(i);
                    unsigned int blasIndex = tlas->GetInstanceBLASIndexByInstanceIndex(i);
                    BLAS* blas = Raytracing::GetBLASByIndex(blasIndex);
                    if (blas) {
                        for (BVHNode& node : blas->bvhNodes) {
                            if (node.IsLeaf()) {
                                AABB aabb(node.aabbMin, node.aabbMax);
                                DrawAABB(aabb, RED, worldTransform);
                            }
                        }
                    }
                }
            }
        }
        else if (g_debugLineRenderMode == DebugLineRenderMode::RTX_LAND_TOP_LEVEL_ACCELERATION_STRUCTURE) {
            TLAS* tlas = Raytracing::GetTLASByIndex(0);
            if (tlas) {
                for (BVHNode& node : tlas->GetNodes()) {
                    AABB tlasAabb(node.aabbMin, node.aabbMax);
                    DrawAABB(tlasAabb, YELLOW);
                }
            }
        }
        else if (g_debugLineRenderMode == DebugLineRenderMode::PATHFINDING_RECAST) {
            // Nav mesh from Recast
            if (true) {
                for (glm::vec3& position : Pathfinding2::GetDebugVertices()) {
                    vertices.push_back(Vertex(position, GREEN));
                }
            }
            // Doberman path
            for (Dobermann& dobermann : Scene::g_dobermann) {
                if (dobermann.m_pathToTarget.points.size() >= 2) {
                    for (int i = 0; i < dobermann.m_pathToTarget.points.size() - 1; i++) {
                        vertices.push_back(Vertex(dobermann.m_pathToTarget.points[i], WHITE));
                        vertices.push_back(Vertex(dobermann.m_pathToTarget.points[i + 1], WHITE));
                    }
                }
            }
        }


    }



    /*
    for (auto& csgObject : CSG::GetCSGObjects()) {
        if (csgObject.m_materialIndex == AssetManager::GetMaterialIndex("FloorBoards")) {
            AABB aabb;
            float xMin = csgObject.m_transform.position.x - csgObject.m_transform.scale.x * 0.5f;
            float yMin = csgObject.m_transform.position.y + csgObject.m_transform.scale.y * 0.5f;
            float zMin = csgObject.m_transform.position.z - csgObject.m_transform.scale.z * 0.5f;
            float xMax = csgObject.m_transform.position.x + csgObject.m_transform.scale.x * 0.5f;
            float yMax = yMin + 2.6f;
            float zMax = csgObject.m_transform.position.z + csgObject.m_transform.scale.z * 0.5f;
            aabb.boundsMin = glm::vec3(xMin, yMin, zMin);
            aabb.boundsMax = glm::vec3(xMax, yMax, zMax);
            DrawAABB(aabb, YELLOW);
        }
    }*/


    //DrawAABB(g_testAABB, YELLOW);
    //DrawSphere(g_testSphere, 12, YELLOW);



    Sphere sphere;
    sphere.origin = Scene::g_lights[3].position;
    sphere.radius = Scene::g_lights[3].radius;
   // DrawSphere(sphere, 12, YELLOW);





     // Tile boundaries (in screen space)
    int x1 = static_cast<int>(PRESENT_WIDTH * 0.6);
    int x2 = static_cast<int>(PRESENT_WIDTH * 0.75f);
    int y1 = static_cast<int>(PRESENT_HEIGHT * 0.6);
    int y2 = static_cast<int>(PRESENT_HEIGHT * 0.75f);

    x2 = x1 + 12;
    y2 = y1 + 12;

    // x1 = static_cast<int>(PRESENT_WIDTH * 0.5f);
    // x2 = static_cast<int>(PRESENT_WIDTH);
    // y1 = static_cast<int>(0);
    // y2 = static_cast<int>(PRESENT_HEIGHT);

    Vertex x1y1, x2y1, x1y2, x2y2;
   //x1y1.position = PixelToNDC(x1, y1, PRESENT_WIDTH, PRESENT_HEIGHT);
   //x2y1.position = PixelToNDC(x2, y1, PRESENT_WIDTH, PRESENT_HEIGHT);
   //x1y2.position = PixelToNDC(x1, y2, PRESENT_WIDTH, PRESENT_HEIGHT);
   //x2y2.position = PixelToNDC(x2, y2, PRESENT_WIDTH, PRESENT_HEIGHT);
    glm::vec3 color = RED;

    y1 = PRESENT_HEIGHT - y1;
    y2 = PRESENT_HEIGHT - y2;

    std::string text;
    text += "\n";




    glm::mat4 projection = Game::GetPlayerByIndex(0)->GetProjectionMatrix();
    glm::mat4 view = Game::GetPlayerByIndex(0)->GetViewMatrix();
    float nearPlane = 0.1f;
    float farPlane = 100.0f;
    float fov = Game::GetPlayerByIndex(0)->GetZoom(); // Horizontal FOV

    float screenWidth = static_cast<float>(PRESENT_WIDTH);
    float screenHeight = static_cast<float>(PRESENT_HEIGHT);
    float viewportWidth = static_cast<float>(PRESENT_WIDTH);
    float viewportHeight = static_cast<float>(PRESENT_HEIGHT);

    Frustum frustum;
    frustum.UpdateFromTile(view, fov, nearPlane, farPlane, x1, y1, x2, y2, viewportWidth, viewportHeight);










    vertices.insert(std::end(vertices), std::begin(g_debugLines), std::end(g_debugLines));
    g_debugLines.clear();

    for (int i = 0; i < vertices.size(); i++) {
        indices.push_back(i);
    }
    g_debugLinesMesh.UpdateVertexBuffer(vertices, indices);
}


glm::mat4 CreatePerspectiveProjectionMatrix(int x1, int x2, int y1, int y2, float nearPlane, float farPlane, float screenWidth, float screenHeight) {
    float ndcX1 = (2.0f * x1 / screenWidth) - 1.0f;
    float ndcX2 = (2.0f * x2 / screenWidth) - 1.0f;
    float ndcY1 = 1.0f - (2.0f * y1 / screenHeight);
    float ndcY2 = 1.0f - (2.0f * y2 / screenHeight);
    float fov = Game::GetPlayerByIndex(0)->m_cameraZoom;
    float tanHalfFov = tan(fov / 2.0f);
    float left = ndcX1 * nearPlane * tanHalfFov;
    float right = ndcX2 * nearPlane * tanHalfFov;
    float bottom = ndcY2 * nearPlane * tanHalfFov;
    float top = ndcY1 * nearPlane * tanHalfFov;
    glm::mat4 projectionMatrix = glm::frustum(left, right, bottom, top, nearPlane, farPlane);
    return projectionMatrix;
}

glm::vec4 NormalizePlane(const glm::vec4& plane) {
    float length = glm::length(glm::vec3(plane));
    return plane / length;
}

// Function to extract the left frustum plane from the view-projection matrix
glm::vec4 ExtractLeftFrustumPlane(const glm::mat4& viewProj) {
    // Left plane: viewProj[3] + viewProj[0]
    glm::vec4 leftPlane = viewProj[3] + viewProj[0];
    return NormalizePlane(leftPlane);
}


void Renderer::UpdateDebugLinesMesh2D() {

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;


    /*
    // Tile boundaries (in screen space)
    int x1 = static_cast<int>(PRESENT_WIDTH * 0.6);
    int x2 = static_cast<int>(PRESENT_WIDTH * 0.75f);
    int y1 = static_cast<int>(PRESENT_HEIGHT * 0.6);
    int y2 = static_cast<int>(PRESENT_HEIGHT * 0.75f);

    x2 = x1 + 12;
    y2 = y1 + 12;

   // x1 = static_cast<int>(PRESENT_WIDTH * 0.5f);
   // x2 = static_cast<int>(PRESENT_WIDTH);
   // y1 = static_cast<int>(0);
   // y2 = static_cast<int>(PRESENT_HEIGHT);

    Vertex x1y1, x2y1, x1y2, x2y2;
    x1y1.position = PixelToNDC(x1, y1, PRESENT_WIDTH, PRESENT_HEIGHT);
    x2y1.position = PixelToNDC(x2, y1, PRESENT_WIDTH, PRESENT_HEIGHT);
    x1y2.position = PixelToNDC(x1, y2, PRESENT_WIDTH, PRESENT_HEIGHT);
    x2y2.position = PixelToNDC(x2, y2, PRESENT_WIDTH, PRESENT_HEIGHT);
    glm::vec3 color = RED;

    y1 = PRESENT_HEIGHT - y1;
    y2 = PRESENT_HEIGHT - y2;

    std::string text;
    text += "\n";

    glm::mat4 projection = Game::GetPlayerByIndex(0)->GetProjectionMatrix();
    glm::mat4 view = Game::GetPlayerByIndex(0)->GetViewMatrix();
    float nearPlane = 0.1f;
    float farPlane = 1.0f;
    float fov = Game::GetPlayerByIndex(0)->GetZoom(); // Horizontal FOV

    float screenWidth = static_cast<float>(PRESENT_WIDTH);
    float screenHeight = static_cast<float>(PRESENT_HEIGHT);
    float viewportWidth = static_cast<float>(PRESENT_WIDTH);
    float viewportHeight = static_cast<float>(PRESENT_HEIGHT);


    // Define the test point (center of the sphere) in world space
    glm::vec3 testPoint(0.0f, 1.0f, 0.0f);


    Frustum frustum;
    frustum.UpdateFromTile(view, fov, nearPlane, farPlane, x1, y1, x2, y2, viewportWidth, viewportHeight);
  //  frustum.UpdateFromTile(view, fov, nearPlane, farPlane, 0, 0, viewportWidth, viewportHeight, viewportWidth, viewportHeight);

    color = RED;

    if (frustum.IntersectsAABB(g_testAABB)) {
        text += "The AABB is inside the frustum.\n";
        color = GREEN;
    }
    else {
        text += "The AABB is outside the frustum.\n";
    }
    if (frustum.IntersectsAABBFast(g_testAABB)) {
        text += "The FAST AABB is inside the frustum.\n";
        color = GREEN;
    }
    else {
        text += "The FAST AABB is outside the frustum.\n";
    }
    if (frustum.IntersectsSphere(g_testSphere)) {
        text += "The Sphere is inside the frustum.\n";
        color = BLUE;
    }
    else {
        text += "The Sphere is outside the frustum.\n";
    }
    if (frustum.IntersectsPoint(testPoint)) {
        text += "White point in frustum\n";
    }
    else {
        text += "White point is NOT frustum\n";
    }



    Player* player = Game::GetPlayerByIndex(0);
    Frustum playerFrustum;
    playerFrustum.Update(player->GetProjectionMatrix() * player->GetViewMatrix());
   if (playerFrustum.IntersectsAABB(g_testAABB)) {
        text += "AABB in player frustum\n";
    }
    else {
        text += "AABB is NOT player frustum\n";
    }
    if (playerFrustum.IntersectsSphere(g_testSphere)) {
        text += "Sphere in player frustum\n";
    }
    else {
        text += "Sphere is NOT player frustum\n";
    }
    if (playerFrustum.IntersectsPoint(testPoint)) {
        text += "White point in frustum\n";
    }
    else {
        text += "White point is NOT frustum\n";
    }

   
    Game::GetPlayerByIndex(0)->_playerName = text;


    x1y1.normal = color;
    x2y1.normal = color;
    x1y2.normal = color;
    x2y2.normal = color;

   //vertices.push_back(x1y1);
   //vertices.push_back(x2y1);
   //
   //vertices.push_back(x1y2);
   //vertices.push_back(x2y2);
   //
   //vertices.push_back(x1y1);
   //vertices.push_back(x1y2);
   //
   //vertices.push_back(x2y1);
   //vertices.push_back(x2y2);
    */


    for (int i = 0; i < vertices.size(); i++) {
        indices.push_back(i);
    }
    g_debugLinesMesh2D.UpdateVertexBuffer(vertices, indices);



}


/*
 ▀█▀ █▀▄ ▀█▀ █▀█ █▀█ █▀▀ █   █▀▀ █▀▀
  █  █▀▄  █  █▀█ █ █ █ █ █   █▀▀ ▀▀█
  ▀  ▀ ▀ ▀▀▀ ▀ ▀ ▀ ▀ ▀▀▀ ▀▀▀ ▀▀▀ ▀▀▀ */

void Renderer::UpdateDebugTrianglesMesh() {

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    // Nav mesh input vertices
    if (false) {
        for (glm::vec3& position : CSG::GetNavMeshVertices()) {
            vertices.push_back(Vertex(position, RED));
        }
    }
    // Nav mesh from Recast
    if (false) {
        for (glm::vec3& position : Pathfinding2::GetDebugVertices()) {
            vertices.push_back(Vertex(position, RED));
        }
        std::cout << Pathfinding2::GetDebugVertices().size() << "\n";
    }

    for (int i = 0; i < vertices.size(); i++) {
        indices.push_back(i);
    }

    g_debugTrianglesMesh.UpdateVertexBuffer(vertices, indices);
}


FacingDirection DetermineFacingDirection(const glm::vec3& forwardVector, const glm::vec3& point, const glm::vec3& origin) {
    glm::vec3 pointVector = point - origin;
    glm::vec3 forwardNormalized = glm::normalize(forwardVector);
    glm::vec3 pointNormalized = glm::normalize(pointVector);
    glm::vec3 rightVector = glm::vec3(forwardNormalized.z, 0.0f, -forwardNormalized.x);
    float dotProduct = glm::dot(pointNormalized, rightVector);
    if (dotProduct > 0) {
        return FacingDirection::LEFT;
    }
    else if (dotProduct < 0) {
        return FacingDirection::RIGHT;
    }
    // The point is directly in front or behind
    return FacingDirection::ALIGNED;
}


/*
 ▀█▀ █▀▀ █ █ ▀█▀
  █  █▀▀ ▄▀▄  █
  ▀  ▀▀▀ ▀ ▀  ▀  */

std::string& Renderer::GetDebugText() {

    g_debugText = "";
    /*
    text += "Splitscreen mode: " + Util::SplitscreenModeToString(Game::GetSplitscreenMode()) + "\n";
    text += "Render mode: " + Util::RenderModeToString(_renderMode) + "\n";
    //text += "Cam pos: " + Util::Vec3ToString(Game::GetPlayerByIndex(i)->GetViewPos()) + "\n";
    text += "Weapon Action: " + Util::WeaponActionToString(Game::GetPlayerByIndex(i)->GetWeaponAction()) + "\n";

    Player* player = Game::GetPlayerByIndex(i);

    text += "\n";
    text += "Current weapon index: " + std::to_string(player->m_currentWeaponIndex) + "\n\n";
    for (int i = 0; i < player->m_weaponStates.size(); i++) {

        if (player->m_weaponStates[i].has) {
            std::string str;
            str += player->m_weaponStates[i].name;
            str += " ";
            str += "\n";
            if (i == player->m_currentWeaponIndex) {
                str = ">" + str;
            }
            else {
                str = " " + str;
            }
            text += str;
        }
    }
    text += "\nAMMO\n";
    for (int i = 0; i < player->m_ammoStates.size(); i++) {
        text += "-";
        text += player->m_ammoStates[i].name;
        text += " ";
        text += std::to_string(player->m_ammoStates[i].ammoOnHand);
        text += "\n";
    }*/


    g_debugText = Util::RenderModeToString(Renderer::GetRenderMode()) + "\n";

    if (Renderer::GetDebugLineRenderMode() != SHOW_NO_LINES) {
        g_debugText += "Line mode: " + Util::DebugLineRenderModeToString(Renderer::GetDebugLineRenderMode()) + "\n";
    }

    g_debugText += "Cam pos: " + Util::Vec3ToString(Game::GetPlayerByIndex(0)->GetViewPos()) + "\n";
    g_debugText += "\n";
    g_debugText += "Dog deaths: " + std::to_string(Game::g_dogDeaths) + "\n";
    g_debugText += "Dog kills: " + std::to_string(Game::g_dogKills) + "\n";
    g_debugText += "Shark deaths: " + std::to_string(Game::g_sharkDeaths) + "\n";
    g_debugText += "Shark kills: " + std::to_string(Game::g_sharkKills) + "\n";
    g_debugText += "\n";

    g_debugText += Scene::GetShark().GetDebugText();



    glm::vec3 rayTarget = Scene::g_lights[0].position;

    std::vector<glm::vec3> vertices;

    for (auto& csgObject : CSG::GetCSGObjects()) {
        std::span<CSGVertex> span = csgObject.GetVerticesSpan();
        for (auto& vertex : span) {
            vertices.push_back(vertex.position);
        }
    }
/*
    Ray ray;
    ray.origin = player->GetViewPos();
    ray.direction = glm::normalize(rayTarget - ray.origin);
    ray.minDist = 0;
    ray.maxDist = glm::distance(rayTarget, ray.origin);;
    */

    /*
    static bool runOnce = true;
    if (runOnce) {
        BVH::UpdateBVH(vertices);
        runOnce = false;
    }

    bool lineOfSight = BVH::AnyHit(ray);

    if (!lineOfSight) {
        g_debugText += "NO line of sight\n";
    }
    else {
        g_debugText += "Line of sight\n";
    }*/


    /*
    static Transform transform;
    transform.position = glm::vec3(0, 1, 0);
    transform.scale = glm::vec3(0.5f, 0.5f, 0.5f);



    glm::mat4 projectionMatrix = player->GetProjectionMatrix();
    glm::mat4 viewMatrix = player->GetViewMatrix();
    glm::mat4 projectionViewMatrix = projectionMatrix * viewMatrix;
    /*
    Frustum frustum;
    frustum.Update(projectionViewMatrix);

    if (frustum.IntersectsAABB(g_testAABB)) {
        g_debugText += "AABB in frustum\n";
    }
    else {
        g_debugText += "AABB not in frustum\n";
    }
    if (frustum.IntersectsSphere(g_testSphere)) {
        g_debugText += "Sphere in frustum\n";
    }
    else {
        g_debugText += "Sphere not in frustum\n";
    }*/

    /*
    for (int i = 0; i < Scene::g_lights.size(); i++) {
        g_debugText += "\n";
        g_debugText += "light " + std::to_string(i) + ": " + std::to_string(Scene::g_lights[i].m_shadowMapIndex) + '\n';
    }*/


  // g_debugText = Util::MenuTypeToString(Editor::GetCurrentMenuType());
  // g_debugText = "";
  // g_debugText += "Hovered object index: " + std::to_string(Editor::g_hoveredObjectIndex) + "\n";
  // g_debugText += "Selected object index: " + std::to_string(Editor::g_selectedObjectIndex) + "\n";
  // g_debugText += "Hovered vertex index: " + std::to_string(Editor::g_hoveredVertexIndex) + "\n";
  // g_debugText += "Selected vertex index: " + std::to_string(Editor::g_selectedVertexIndex) + "\n";
  // g_debugText += "Selected vertex position: " + Util::Vec3ToString(Editor::g_selectedVertexPosition) + "\n";
  // g_debugText += "\n";
  // g_debugText += "Cubes: " + std::to_string(Scene::g_csgAdditiveCubes.size()) + "\n";
  // g_debugText += "Wall planes: " + std::to_string(Scene::g_csgAdditiveWallPlanes.size()) + "\n";
  // g_debugText += "Ceiling planes: " + std::to_string(Scene::g_csgAdditiveCeilingPlanes.size()) + "\n";
  // g_debugText += "\n";
  // g_debugText += Util::Mat4ToString(Editor::g_gizmoMatrix);

    return g_debugText;
}

void Renderer::DrawSphere(const Sphere& sphere, int segments, const glm::vec3& color) {
    // Ensure segments is at least 4 to form a basic sphere
    segments = std::max(segments, 4);
    // Angles for generating circle points
    float theta_step = glm::two_pi<float>() / segments;
    float phi_step = glm::pi<float>() / segments;
    // Iterate over latitude (phi) and longitude (theta)
    for (int i = 0; i <= segments; ++i) {
        float phi = i * phi_step;
        for (int j = 0; j <= segments; ++j) {
            float theta = j * theta_step;
            // Spherical to Cartesian conversion
            glm::vec3 point_on_sphere(
                sphere.radius * sin(phi) * cos(theta),
                sphere.radius * cos(phi),
                sphere.radius * sin(phi) * sin(theta)
            );
            // Transform the point to the sphere's origin
            glm::vec3 current_point = sphere.origin + point_on_sphere;
            // Draw line to the next point in theta direction
            if (j > 0) {
                float prev_theta = (j - 1) * theta_step;
                glm::vec3 prev_point_on_sphere(
                    sphere.radius * sin(phi) * cos(prev_theta),
                    sphere.radius * cos(phi),
                    sphere.radius * sin(phi) * sin(prev_theta)
                );
                glm::vec3 previous_point = sphere.origin + prev_point_on_sphere;
                DrawLine(previous_point, current_point, color);
            }
            // Draw line to the next point in phi direction
            if (i > 0) {
                float prev_phi = (i - 1) * phi_step;
                glm::vec3 prev_point_on_sphere(
                    sphere.radius * sin(prev_phi) * cos(theta),
                    sphere.radius * cos(prev_phi),
                    sphere.radius * sin(prev_phi) * sin(theta)
                );
                glm::vec3 previous_point = sphere.origin + prev_point_on_sphere;
                DrawLine(previous_point, current_point, color);
            }
        }
    }
}

void Renderer::DrawFrustum(Frustum& frustum, glm::vec3 color) {
    std::vector<glm::vec3>  corners = frustum.GetFrustumCorners();
    DrawLine(corners[0], corners[1], color);  // Near bottom edge
    DrawLine(corners[1], corners[3], color);  // Near right edge
    DrawLine(corners[3], corners[2], color);  // Near top edge
    DrawLine(corners[2], corners[0], color);  // Near left edge
    DrawLine(corners[4], corners[5], color);  // Far bottom edge
    DrawLine(corners[5], corners[7], color);  // Far right edge
    DrawLine(corners[7], corners[6], color);  // Far top edge
    DrawLine(corners[6], corners[4], color);  // Far left edge
    DrawLine(corners[0], corners[4], color);  // Bottom-left edge
    DrawLine(corners[1], corners[5], color);  // Bottom-right edge
    DrawLine(corners[2], corners[6], color);  // Top-left edge
    DrawLine(corners[3], corners[7], color);  // Top-right edge
}

void Renderer::DrawLine(glm::vec3 begin, glm::vec3 end, glm::vec3 color) {
    Vertex v0 = Vertex(begin, color);
    Vertex v1 = Vertex(end, color);
    g_debugLines.push_back(v0);
    g_debugLines.push_back(v1);
}

void Renderer::DrawAABB(AABB& aabb, glm::vec3 color) {
    glm::vec3 FrontTopLeft = glm::vec3(aabb.GetBoundsMin().x, aabb.GetBoundsMax().y, aabb.GetBoundsMax().z);
    glm::vec3 FrontTopRight = glm::vec3(aabb.GetBoundsMax().x, aabb.GetBoundsMax().y, aabb.GetBoundsMax().z);
    glm::vec3 FrontBottomLeft = glm::vec3(aabb.GetBoundsMin().x, aabb.GetBoundsMin().y, aabb.GetBoundsMax().z);
    glm::vec3 FrontBottomRight = glm::vec3(aabb.GetBoundsMax().x, aabb.GetBoundsMin().y, aabb.GetBoundsMax().z);
    glm::vec3 BackTopLeft = glm::vec3(aabb.GetBoundsMin().x, aabb.GetBoundsMax().y, aabb.GetBoundsMin().z);
    glm::vec3 BackTopRight = glm::vec3(aabb.GetBoundsMax().x, aabb.GetBoundsMax().y, aabb.GetBoundsMin().z);
    glm::vec3 BackBottomLeft = glm::vec3(aabb.GetBoundsMin().x, aabb.GetBoundsMin().y, aabb.GetBoundsMin().z);
    glm::vec3 BackBottomRight = glm::vec3(aabb.GetBoundsMax().x, aabb.GetBoundsMin().y, aabb.GetBoundsMin().z);
    DrawLine(FrontTopLeft, FrontTopRight, color);
    DrawLine(FrontBottomLeft, FrontBottomRight, color);
    DrawLine(BackTopLeft, BackTopRight, color);
    DrawLine(BackBottomLeft, BackBottomRight, color);
    DrawLine(FrontTopLeft, FrontBottomLeft, color);
    DrawLine(FrontTopRight, FrontBottomRight, color);
    DrawLine(BackTopLeft, BackBottomLeft, color);
    DrawLine(BackTopRight, BackBottomRight, color);
    DrawLine(FrontTopLeft, BackTopLeft, color);
    DrawLine(FrontTopRight, BackTopRight, color);
    DrawLine(FrontBottomLeft, BackBottomLeft, color);
    DrawLine(FrontBottomRight, BackBottomRight, color);
}

void Renderer::DrawAABB(AABB& aabb, glm::vec3 color, glm::mat4 worldTransform) {
    glm::vec3 FrontTopLeft = worldTransform * glm::vec4(aabb.GetBoundsMin().x, aabb.GetBoundsMax().y, aabb.GetBoundsMax().z, 1.0f);
    glm::vec3 FrontTopRight = worldTransform * glm::vec4(aabb.GetBoundsMax().x, aabb.GetBoundsMax().y, aabb.GetBoundsMax().z, 1.0f);
    glm::vec3 FrontBottomLeft = worldTransform * glm::vec4(aabb.GetBoundsMin().x, aabb.GetBoundsMin().y, aabb.GetBoundsMax().z, 1.0f);
    glm::vec3 FrontBottomRight = worldTransform * glm::vec4(aabb.GetBoundsMax().x, aabb.GetBoundsMin().y, aabb.GetBoundsMax().z, 1.0f);
    glm::vec3 BackTopLeft = worldTransform * glm::vec4(aabb.GetBoundsMin().x, aabb.GetBoundsMax().y, aabb.GetBoundsMin().z, 1.0f);
    glm::vec3 BackTopRight = worldTransform * glm::vec4(aabb.GetBoundsMax().x, aabb.GetBoundsMax().y, aabb.GetBoundsMin().z, 1.0f);
    glm::vec3 BackBottomLeft = worldTransform * glm::vec4(aabb.GetBoundsMin().x, aabb.GetBoundsMin().y, aabb.GetBoundsMin().z, 1.0f);
    glm::vec3 BackBottomRight = worldTransform * glm::vec4(aabb.GetBoundsMax().x, aabb.GetBoundsMin().y, aabb.GetBoundsMin().z, 1.0f);
    DrawLine(FrontTopLeft, FrontTopRight, color);
    DrawLine(FrontBottomLeft, FrontBottomRight, color);
    DrawLine(BackTopLeft, BackTopRight, color);
    DrawLine(BackBottomLeft, BackBottomRight, color);
    DrawLine(FrontTopLeft, FrontBottomLeft, color);
    DrawLine(FrontTopRight, FrontBottomRight, color);
    DrawLine(BackTopLeft, BackBottomLeft, color);
    DrawLine(BackTopRight, BackBottomRight, color);
    DrawLine(FrontTopLeft, BackTopLeft, color);
    DrawLine(FrontTopRight, BackTopRight, color);
    DrawLine(FrontBottomLeft, BackBottomLeft, color);
    DrawLine(FrontBottomRight, BackBottomRight, color);
}

inline std::vector<DebugLineRenderMode> _allowedDebugLineRenderModes = {
    SHOW_NO_LINES,
    //PATHFINDING,
    PHYSX_COLLISION,
    PATHFINDING_RECAST,
    RTX_LAND_TOP_LEVEL_ACCELERATION_STRUCTURE,
    RTX_LAND_BOTTOM_LEVEL_ACCELERATION_STRUCTURES,
    BOUNDING_BOXES,
};

void Renderer::NextDebugLineRenderMode() {
    g_debugLineRenderMode = (DebugLineRenderMode)(int(g_debugLineRenderMode) + 1);
    if (g_debugLineRenderMode == DEBUG_LINE_MODE_COUNT) {
        g_debugLineRenderMode = (DebugLineRenderMode)0;
    }
    // If mode isn't in available modes list, then go to next
    bool allowed = false;
    for (auto& avaliableMode : _allowedDebugLineRenderModes) {
        if (g_debugLineRenderMode == avaliableMode) {
            allowed = true;
            break;
        }
    }
    if (!allowed && g_debugLineRenderMode != DebugLineRenderMode::SHOW_NO_LINES) {
        NextDebugLineRenderMode();
    }
}

DebugLineRenderMode Renderer::GetDebugLineRenderMode() {
    return g_debugLineRenderMode;
}