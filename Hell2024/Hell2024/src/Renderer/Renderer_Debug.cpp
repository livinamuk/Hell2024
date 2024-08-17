#include "Renderer.h"
#include "../Physics/Physics.h"
#include "../Editor/Editor.h"
#include "../Game/Game.h"
#include "../Game/Scene.h"
#include "../Renderer/GlobalIllumination.h"
#include "../Pathfinding/Pathfinding2.h"
#include "../Renderer/Raytracing/Raytracing.h"
#include "../Math/Frustum.hpp"

/*
 █▀█ █▀█ ▀█▀ █▀█ ▀█▀ █▀▀
 █▀▀ █ █  █  █ █  █  ▀▀█
 ▀   ▀▀▀ ▀▀▀ ▀ ▀  ▀  ▀▀▀ */

std::string g_debugText = "";

void Renderer::UpdateDebugPointsMesh() {

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    Player* player = Game::GetPlayerByIndex(0);

    if (g_debugLineRenderMode == DebugLineRenderMode::PATHFINDING_RECAST) {
        // Dobermann path
        for (Dobermann& dobermann : Scene::g_dobermann) {
            for (int i = 0; i < dobermann.m_pathToPlayer.points.size(); i++) {
                vertices.push_back(Vertex(dobermann.m_pathToPlayer.points[i], RED));
            }
        }
    }

    for (Light& light : Scene::g_lights) {
        for (unsigned int index : light.visibleCloudPointIndices) {
            CloudPoint& cloudPoint = GlobalIllumination::GetPointCloud()[index];
            vertices.push_back(Vertex(cloudPoint.position, GREEN));
        }
    }

    /*
    Light& light = Scene::g_lights[0];
    for (unsigned int index : light.visibleCloudPointIndices) {
        CloudPoint& cloudPoint = GlobalIllumination::GetPointCloud()[index];
        vertices.push_back(Vertex(cloudPoint.position, GREEN));
    }*/




    for (int i = 0; i < vertices.size(); i++) {
        indices.push_back(i);
    }
    g_debugPointsMesh.UpdateVertexBuffer(vertices, indices);
}

/*
 █   ▀█▀ █▀█ █▀▀ █▀▀
 █    █  █ █ █▀▀ ▀▀█
 ▀▀▀ ▀▀▀ ▀ ▀ ▀▀▀ ▀▀▀ */

void Renderer::UpdateDebugLinesMesh() {

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    std::vector<PxRigidActor*> ignoreList;

    // Skip debug lines for player 0 ragdoll
    Player* player = Game::GetPlayerByIndex(0);
    AnimatedGameObject* characterModel = Scene::GetAnimatedGameObjectByIndex(player->GetCharacterModelAnimatedGameObjectIndex());
    for (auto r : characterModel->_ragdoll._rigidComponents) {
        ignoreList.push_back(r.pxRigidBody);
    }

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
            for (GameObject& gameObject : Scene::GetGamesObjects()) {
                std::vector<Vertex> aabbVertices = gameObject.GetAABBVertices();
                vertices.reserve(vertices.size() + aabbVertices.size());
                vertices.insert(std::end(vertices), std::begin(aabbVertices), std::end(aabbVertices));
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
                if (dobermann.m_pathToPlayer.points.size() >= 2) {
                    for (int i = 0; i < dobermann.m_pathToPlayer.points.size() - 1; i++) {
                        vertices.push_back(Vertex(dobermann.m_pathToPlayer.points[i], WHITE));
                        vertices.push_back(Vertex(dobermann.m_pathToPlayer.points[i + 1], WHITE));
                    }
                }
            }
        }


        /*

        for (Dobermann& dobermann : Scene::g_dobermann) {

            if (dobermann.m_aStar.m_finalPathPoints.size() >= 2) {
                for (int i = 0; i < dobermann.m_aStar.m_finalPathPoints.size() - 1; i++) {

                    glm::vec2 point = dobermann.m_aStar.m_finalPathPoints[i];
                    float worldX = point.x * Pathfinding::GetGridSpacing() + Pathfinding::GetWorldSpaceOffsetX();
                    float worldZ = point.y * Pathfinding::GetGridSpacing() + Pathfinding::GetWorldSpaceOffsetZ();
                    vertices.push_back(Vertex(glm::vec3(worldX, 0, worldZ), WHITE));

                    point = dobermann.m_aStar.m_finalPathPoints[i + 1];
                    worldX = point.x * Pathfinding::GetGridSpacing() + Pathfinding::GetWorldSpaceOffsetX();
                    worldZ = point.y * Pathfinding::GetGridSpacing() + Pathfinding::GetWorldSpaceOffsetZ();
                    vertices.push_back(Vertex(glm::vec3(worldX, 0, worldZ), WHITE));

                }
            }
        }*/



        // THE GRID WORKS. U JUST DONT WANNA SEE IT RN.
        // THE GRID WORKS. U JUST DONT WANNA SEE IT RN.
        // THE GRID WORKS. U JUST DONT WANNA SEE IT RN.
        // THE GRID WORKS. U JUST DONT WANNA SEE IT RN.

        /*
        else if (_debugLineRenderMode == DebugLineRenderMode::PATHFINDING) {
            glm::vec3 gridColor = glm::vec3(0.25f);
            for (float x = 0; x < Pathfinding::GetWorldSpaceWidth(); x += Pathfinding::GetGridSpacing()) {
                for (float z = 0; z < Pathfinding::GetWorldSpaceDepth(); z += Pathfinding::GetGridSpacing()) {
                    Vertex v0, v1;
                    v0.position = glm::vec3(x + Pathfinding::GetWorldSpaceOffsetX(), 0, 0 + Pathfinding::GetWorldSpaceOffsetZ());
                    v1.position = glm::vec3(x + Pathfinding::GetWorldSpaceOffsetX(), 0, Pathfinding::GetWorldSpaceDepth() + Pathfinding::GetWorldSpaceOffsetZ());
                    v0.normal = gridColor;
                    v1.normal = gridColor;
                    vertices.push_back(v0);
                    vertices.push_back(v1);
                    v0.position = glm::vec3(0 + Pathfinding::GetWorldSpaceOffsetX(), 0, z + Pathfinding::GetWorldSpaceOffsetZ());
                    v1.position = glm::vec3(Pathfinding::GetWorldSpaceWidth() + Pathfinding::GetWorldSpaceOffsetX(), 0, z + Pathfinding::GetWorldSpaceOffsetZ());
                    v0.normal = gridColor;
                    v1.normal = gridColor;
                    vertices.push_back(v0);
                    vertices.push_back(v1);
                }
            }
        }*/
    }

    // DRAW ALL BLAS
    /*
    for (int i = 0; i < Raytracing::GetBottomLevelAccelerationStructureCount(); i++) {
        BLAS* blas = Raytracing::GetBLASByIndex(i);
        if (blas) {
            for (Triangle& triangle : blas->triangles) {
                Vertex v0;
                Vertex v1;
                Vertex v2;
                v0.normal = YELLOW;
                v1.normal = YELLOW;
                v2.normal = YELLOW;
                v0.position = triangle.v0;
                v1.position = triangle.v1;
                v2.position = triangle.v2;
                vertices.push_back(v0);
                vertices.push_back(v1);
                vertices.push_back(v1);
                vertices.push_back(v2);
                vertices.push_back(v2);
                vertices.push_back(v0);
            }
        }
    }*/



    /*
    glm::mat4 projection = Game::GetPlayerByIndex(0)->GetProjectionMatrix();
    glm::mat4 view = Editor::GetViewMatrix();
    glm::vec3 rayOrigin = Editor::GetViewPos();
    glm::vec3 rayDir = Game::GetPlayerByIndex(0)->GetCameraForward();
    glm::vec3 color = RED;
    int viewportWidth = PRESENT_WIDTH;
    int viewportHeight = PRESENT_HEIGHT;
    float mouseX = Util::MapRange(Input::GetMouseX(), 0, BackEnd::GetCurrentWindowWidth(), 0, viewportWidth);
    float mouseY = Util::MapRange(Input::GetMouseY(), 0, BackEnd::GetCurrentWindowHeight(), 0, viewportHeight);
    rayDir = Math::GetMouseRay(projection, view, viewportWidth, viewportHeight, mouseX, mouseY);

    IntersectionResult intersectionResult;
    for (int i = 0; i < g_debugTriangleVertices.size(); i += 3) {
        glm::vec3 v0 = g_debugTriangleVertices[i];
        glm::vec3 v1 = g_debugTriangleVertices[i + 1];
        glm::vec3 v2 = g_debugTriangleVertices[i + 2];
        intersectionResult = Math::RayTriangleIntersectTest(rayOrigin, rayDir, v0, v1, v2);
    }

    std::vector<glm::vec3> closestTri = Math::ClosestTriangleRayIntersection(rayOrigin, rayDir, g_debugTriangleVertices);

    for (int i = 0; i < g_debugTriangleVertices.size(); i += 3) {
        Vertex v0;
        Vertex v1;
        Vertex v2;
        v0.normal = RED;
        v1.normal = RED;
        v2.normal = RED;
        v0.position = g_debugTriangleVertices[i];
        v1.position = g_debugTriangleVertices[i + 1];
        v2.position = g_debugTriangleVertices[i + 2];
        vertices.push_back(v0);
        vertices.push_back(v1);
        vertices.push_back(v1);
        vertices.push_back(v2);
        vertices.push_back(v2);
        vertices.push_back(v0);
    }
    for (int i = 0; i < closestTri.size(); i += 3) {
        Vertex v0;
        Vertex v1;
        Vertex v2;
        v0.normal = YELLOW;
        v1.normal = YELLOW;
        v2.normal = YELLOW;
        v0.position = closestTri[i];
        v1.position = closestTri[i + 1];
        v2.position = closestTri[i + 2];
        vertices.push_back(v0);
        vertices.push_back(v1);
        vertices.push_back(v1);
        vertices.push_back(v2);
        vertices.push_back(v2);
        vertices.push_back(v0);
    }*/


    /*
    glm::vec3 aabbMin = glm::vec3(-0.25f, 0.75f, -0.25f);
    glm::vec3 aabbMax = glm::vec3(0.25f, 1.25f, 0.25f);
    AABB aabb(aabbMin, aabbMax);
    DrawAABB(aabb, YELLOW);
    Frustum frustm;
    std::vector<Vertex> cornersLineVertices = frustm.GetFrustumCornerLineVertices(player->GetProjectionMatrix(), player->GetViewMatrix(), GREEN);
    vertices.insert(std::end(vertices), std::begin(cornersLineVertices), std::end(cornersLineVertices));
    */


    vertices.insert(std::end(vertices), std::begin(g_debugLines), std::end(g_debugLines));
    g_debugLines.clear();

    for (int i = 0; i < vertices.size(); i++) {
        indices.push_back(i);
    }
    g_debugLinesMesh.UpdateVertexBuffer(vertices, indices);
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

    if (Renderer::GetRenderMode() == COMPOSITE) {
        g_debugText = "Direct + Indirect Light\n";
    }
    else if (Renderer::GetRenderMode() == DIRECT_LIGHT) {
        g_debugText = "Direct Light\n";
    }
    else if (Renderer::GetRenderMode() == COMPOSITE_PLUS_POINT_CLOUD) {
        g_debugText = "Direct + Indirect Light + Point cloud\n";
    }
    else if (Renderer::GetRenderMode() == POINT_CLOUD) {
        g_debugText = "Point Cloud\n";
    }
    if (Renderer::GetDebugLineRenderMode() != SHOW_NO_LINES) {
        g_debugText += "Line mode: " + Util::DebugLineRenderModeToString(Renderer::GetDebugLineRenderMode()) + "\n";
    }
    if (Editor::IsOpen()) {
        g_debugText = Editor::GetDebugText();
    }
    /*
    Player* player = Game::GetPlayerByIndex(0);
    glm::mat4 projectionMatrix = player->GetProjectionMatrix();
    glm::mat4 viewMatrix = player->GetViewMatrix();
    glm::vec3 viewPos = player->GetViewPos();
    glm::vec3 viewRot = player->GetViewRotation();
    g_debugText += Util::Mat4ToString(projectionMatrix) + "\n\n";
    g_debugText += Util::Mat4ToString(viewMatrix) + "\n\n";
    g_debugText += "View pos: " + Util::Vec3ToString(viewPos) + "\n";
    g_debugText += "View rot: " + Util::Vec3ToString(viewRot) + "\n";
    //g_debugText += player->_playerName + "\n";

    glm::vec3 forward = Game::GetPlayerByIndex(0)->GetCameraForward();
    glm::vec3 origin = Game::GetPlayerByIndex(0)->GetFeetPosition();
    glm::vec3 point = Game::GetPlayerByIndex(1)->GetFeetPosition();

    FacingDirection facingDirection = DetermineFacingDirection(forward, origin, point);
    if (facingDirection == FacingDirection::LEFT) {
        g_debugText += "Facing: LEFT\n";
    }
    if (facingDirection == FacingDirection::RIGHT) {
        g_debugText += "Facing: RIGHT\n";
    }
    if (facingDirection == FacingDirection::ALIGNED) {
        g_debugText += "Facing: ALIGNED\n";
    }*/

    Player* player = Game::GetPlayerByIndex(0);
    Frustum frustum;
    frustum.Update(player->GetProjectionMatrix(), player->GetViewMatrix());

    glm::vec3 extent = glm::vec3(0.5f);
    glm::vec3 center = glm::vec3(0.0f);

    glm::vec3 aabbMin = glm::vec3(-0.25f, 0.75f, -0.25f);
    glm::vec3 aabbMax = glm::vec3(0.25f, 1.25f, 0.25f);

    if (frustum.IsAABBInsideFrustum(aabbMin, aabbMax)) {
        g_debugText = "AABB in frustum\n";
    }
    else {
        g_debugText = "AABB is NOT frustum\n";
    }

    /*
    static Transform transform;
    transform.position = glm::vec3(0, 1, 0);
    transform.scale = glm::vec3(0.5f, 0.5f, 0.5f);




    text += "\n";
    std::vector<glm::vec3> corners = frustum.GetFrustumCorners(player->GetProjectionMatrix(), player->GetViewMatrix());
    for (int i = 0; i < corners.size(); i++) {
        text += Util::Vec3ToString10(corners[i]) + "\n";
    }


    text += "\n";
    glm::mat4 projectionMatrix = player->GetProjectionMatrix();
    glm::mat4 viewMatrix = player->GetViewMatrix();
    glm::mat4 inverseViewMatrix = glm::inverse(viewMatrix);
    glm::vec3 viewPos = glm::vec3(inverseViewMatrix[3][0], inverseViewMatrix[3][1], inverseViewMatrix[3][2]);
    glm::vec3 camForward = -glm::vec3(viewMatrix[0][2], viewMatrix[1][2], viewMatrix[2][2]);
    glm::vec3 camRight = glm::vec3(viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0]);
    glm::vec3 camUp = glm::vec3(viewMatrix[0][1], viewMatrix[1][1], viewMatrix[2][1]);
    float fov = 2.0f * atan(1.0f / projectionMatrix[1][1]);
    float aspectRatio = projectionMatrix[1][1] / projectionMatrix[0][0];
    float nearPlane = projectionMatrix[3][2] / (projectionMatrix[2][2] - 1.0f);
    float farPlane = projectionMatrix[3][2] / (projectionMatrix[2][2] + 1.0f);
    glm::vec3 fc = viewPos + camForward * farPlane;
    glm::vec3 nc = viewPos + camForward * nearPlane;
    float Hfar = 2.0f * tan(fov / 2) * farPlane;
    float Wfar = Hfar * aspectRatio;
    float Hnear = 2.0f * tan(fov / 2) * nearPlane;
    float Wnear = Hnear * aspectRatio;
    glm::vec3 up = camUp;
    glm::vec3 right = camRight;
    glm::vec3 ftl = fc + (up * Hfar / 2.0f) - (right * Wfar / 2.0f);
    glm::vec3 ftr = fc + (up * Hfar / 2.0f) + (right * Wfar / 2.0f);
    glm::vec3 fbl = fc - (up * Hfar / 2.0f) - (right * Wfar / 2.0f);
    glm::vec3 fbr = fc - (up * Hfar / 2.0f) + (right * Wfar / 2.0f);
    glm::vec3 ntl = nc + (up * Hnear / 2.0f) - (right * Wnear / 2.0f);
    glm::vec3 ntr = nc + (up * Hnear / 2.0f) + (right * Wnear / 2.0f);
    glm::vec3 nbl = nc - (up * Hnear / 2.0f) - (right * Wnear / 2.0f);
    glm::vec3 nbr = nc - (up * Hnear / 2.0f) + (right * Wnear / 2.0f);


    text += Util::Mat4ToString10(projectionMatrix) + "\n\n";
    text += Util::Mat4ToString10(viewMatrix) + "\n\n";

    text += Util::Mat4ToString(projectionMatrix) + "\n\n";
    text += Util::Mat4ToString(viewMatrix) + "\n";
    */

    /*
    const Plane* planes[] = {
        &frustum.topFace,
        &frustum.bottomFace,
        &frustum.rightFace,
        &frustum.leftFace,
        &frustum.farFace,
        &frustum.nearFace
    };
    text += "\n";
    for (const Plane* plane : planes) {
        glm::vec3 positiveVertex = aabbMin;
        if (plane->normal.x >= 0) {
            positiveVertex.x = aabbMax.x;
        }
        if (plane->normal.y >= 0) {
            positiveVertex.y = aabbMax.y;
        }
        if (plane->normal.z >= 0) {
            positiveVertex.z = aabbMax.z;
        }
        text += "normal: " + Util::Vec3ToString10(plane->normal) + "\n";
        text += "distance: " + std::to_string(plane->distance) + "\n";
        text += "positiveVertex: " + Util::Vec3ToString10(positiveVertex) + "\n";
        text += "signedDist: " + std::to_string(plane->GetSignedDistanceToPlane(positiveVertex));

        if (plane->GetSignedDistanceToPlane(positiveVertex) < 0) {
            text += "\n";
        }
        else {
            text += "   BELOW 0 \n";
        }
    }*/


    //g_debugText = "";


    return g_debugText;
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