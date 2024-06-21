#include "Wall.h"
#include "ModularCommon.h"
#include "../../Core/AssetManager.h"
#include "../../Core/Scene.h"

float RoundToTwoDecimalPlaces(float var) {
    float value = (int)(var * 100 + .5);
    return (float)value / 100;
}

Wall::Wall(glm::vec3 begin, glm::vec3 end, float height, int materialIndex) {

    begin.x = RoundToTwoDecimalPlaces(begin.x);
    begin.y = RoundToTwoDecimalPlaces(begin.y);
    begin.z = RoundToTwoDecimalPlaces(begin.z);
    end.x = RoundToTwoDecimalPlaces(end.x);
    end.y = RoundToTwoDecimalPlaces(end.y);
    end.z = RoundToTwoDecimalPlaces(end.z);

    this->materialIndex = materialIndex;
    this->begin = begin;
    this->end = end;
    this->height = height;
}

void Wall::CreateVertexData() {

    vertices.clear();

    ceilingTrims.clear();
    floorTrims.clear();
    collisionLines.clear();

    // Init shit
    bool finishedBuildingWall = false;
    glm::vec3 wallStart = begin;
    glm::vec3 wallEnd = end;
    glm::vec3 cursor = wallStart;
    glm::vec3 wallDir = glm::normalize(wallEnd - cursor);
    float texScale = 2.0f;
    if (materialIndex == AssetManager::GetMaterialIndex("WallPaper")) {
        texScale = 1.0f;
    }

    float uvX1 = 0;
    float uvX2 = 0;

    bool hasTrims = (height == WALL_HEIGHT);

    int count = 0;
    while (!finishedBuildingWall || count > 1000) {
        count++;
        float shortestDistance = 9999;
        Door* closestDoor = nullptr;
        Window* closestWindow = nullptr;
        glm::vec3 intersectionPoint;

        for (Door& door : Scene::_doors) {

            // Left side
            glm::vec3 v1(door.GetFloorplanVertFrontLeft(0.05f));
            glm::vec3 v2(door.GetFloorplanVertBackRight(0.05f));
            // Right side
            glm::vec3 v3(door.GetFloorplanVertBackLeft(0.05f));
            glm::vec3 v4(door.GetFloorplanVertFrontRight(0.05f));
            // If an intersection is found closer than one u have already then store it
            glm::vec3 tempIntersectionPoint;
            if (Util::LineIntersects(v1, v2, cursor, wallEnd, tempIntersectionPoint)) {
                if (shortestDistance > glm::distance(cursor, tempIntersectionPoint)) {
                    shortestDistance = glm::distance(cursor, tempIntersectionPoint);
                    closestDoor = &door;
                    intersectionPoint = tempIntersectionPoint;
                }
            }
            // Check the other side now
            if (Util::LineIntersects(v3, v4, cursor, wallEnd, tempIntersectionPoint)) {
                if (shortestDistance > glm::distance(cursor, tempIntersectionPoint)) {
                    shortestDistance = glm::distance(cursor, tempIntersectionPoint);
                    closestDoor = &door;
                    intersectionPoint = tempIntersectionPoint;
                }
            }
        }

        for (Window& window : Scene::_windows) {

            // Left side
            glm::vec3 v3(window.GetFrontLeftCorner());
            glm::vec3 v4(window.GetBackRightCorner());
            // Right side
            glm::vec3 v1(window.GetFrontRightCorner());
            glm::vec3 v2(window.GetBackLeftCorner());

            v1.y = 0.1f;
            v2.y = 0.1f;
            v3.y = 0.1f;
            v4.y = 0.1f;

            // If an intersection is found closer than one u have already then store it
            glm::vec3 tempIntersectionPoint;
            if (Util::LineIntersects(v1, v2, cursor, wallEnd, tempIntersectionPoint)) {
                if (shortestDistance > glm::distance(cursor, tempIntersectionPoint)) {
                    shortestDistance = glm::distance(cursor, tempIntersectionPoint);
                    closestWindow = &window;
                    closestDoor = NULL;
                    intersectionPoint = tempIntersectionPoint;
                    //std::cout << "\n\n\nHELLO\n\n";
                }
            }
            // Check the other side now
            if (Util::LineIntersects(v3, v4, cursor, wallEnd, tempIntersectionPoint)) {
                if (shortestDistance > glm::distance(cursor, tempIntersectionPoint)) {
                    shortestDistance = glm::distance(cursor, tempIntersectionPoint);
                    closestWindow = &window;
                    closestDoor = NULL;
                    intersectionPoint = tempIntersectionPoint;
                }
            }
        }

        // Did ya find a door?
        if (closestDoor != nullptr) {

            // The wall piece from cursor to door
            Vertex v1, v2, v3, v4;
            v1.position = cursor;
            v2.position = cursor + glm::vec3(0, height, 0);
            v3.position = intersectionPoint + glm::vec3(0, height, 0);
            v4.position = intersectionPoint;
            float segmentWidth = abs(glm::length((v4.position - v1.position))) / WALL_HEIGHT;
            float segmentHeight = glm::length((v2.position - v1.position)) / WALL_HEIGHT;
            uvX2 = uvX1 + segmentWidth;
            v1.uv = glm::vec2(uvX1, segmentHeight) * texScale;
            v2.uv = glm::vec2(uvX1, 0) * texScale;
            v3.uv = glm::vec2(uvX2, 0) * texScale;
            v4.uv = glm::vec2(uvX2, segmentHeight) * texScale;
            SetNormalsAndTangentsFromVertices(&v3, &v2, &v1);
            SetNormalsAndTangentsFromVertices(&v3, &v1, &v4);
            vertices.push_back(v3);
            vertices.push_back(v2);
            vertices.push_back(v1);
            vertices.push_back(v3);
            vertices.push_back(v1);
            vertices.push_back(v4);

            if (hasTrims) {
                Transform trimTransform;
                trimTransform.position = cursor;
                trimTransform.rotation.y = Util::YRotationBetweenTwoPoints(v4.position, v1.position) + HELL_PI;
                trimTransform.scale.x = segmentWidth * WALL_HEIGHT;
                ceilingTrims.push_back(trimTransform);
                floorTrims.push_back(trimTransform);
            }

            // Bit above the door
            Vertex v5, v6, v7, v8;
            v5.position = intersectionPoint + glm::vec3(0, DOOR_HEIGHT, 0);
            v6.position = intersectionPoint + glm::vec3(0, height, 0);
            v7.position = intersectionPoint + (wallDir * (DOOR_WIDTH + 0.005f)) + glm::vec3(0, height, 0);
            v8.position = intersectionPoint + (wallDir * (DOOR_WIDTH + 0.005f)) + glm::vec3(0, DOOR_HEIGHT, 0);
            segmentWidth = abs(glm::length((v8.position - v5.position))) / WALL_HEIGHT;
            segmentHeight = glm::length((v6.position - v5.position)) / WALL_HEIGHT;
            uvX1 = uvX2;
            uvX2 = uvX1 + segmentWidth;
            v5.uv = glm::vec2(uvX1, segmentHeight) * texScale;
            v6.uv = glm::vec2(uvX1, 0) * texScale;
            v7.uv = glm::vec2(uvX2, 0) * texScale;
            v8.uv = glm::vec2(uvX2, segmentHeight) * texScale;
            SetNormalsAndTangentsFromVertices(&v7, &v6, &v5);
            SetNormalsAndTangentsFromVertices(&v7, &v5, &v8);
            vertices.push_back(v7);
            vertices.push_back(v6);
            vertices.push_back(v5);
            vertices.push_back(v7);
            vertices.push_back(v5);
            vertices.push_back(v8);

            if (hasTrims) {
                Transform trimTransform;
                trimTransform.position = intersectionPoint;
                trimTransform.rotation.y = Util::YRotationBetweenTwoPoints(v4.position, v1.position) + HELL_PI;
                trimTransform.scale.x = segmentWidth * WALL_HEIGHT;
                ceilingTrims.push_back(trimTransform);
            }

            cursor = intersectionPoint + (wallDir * (DOOR_WIDTH + 0.005f)); // This 0.05 is so you don't get an intersection with the door itself
            uvX1 = uvX2;

            Line collisionLine;
            collisionLine.p1.pos = v1.position;
            collisionLine.p2.pos = v4.position;
            collisionLine.p1.color = YELLOW;
            collisionLine.p2.color = RED;
            collisionLines.push_back(collisionLine);
        }
        else if (closestWindow != nullptr) {

            // The wall piece from cursor to window
            Vertex v1, v2, v3, v4;
            v1.position = cursor;
            v2.position = cursor + glm::vec3(0, height, 0);
            v3.position = intersectionPoint + glm::vec3(0, height, 0);
            v4.position = intersectionPoint;
            float segmentWidth = abs(glm::length((v4.position - v1.position))) / WALL_HEIGHT;
            float segmentHeight = glm::length((v2.position - v1.position)) / WALL_HEIGHT;
            uvX2 = uvX1 + segmentWidth;
            v1.uv = glm::vec2(uvX1, segmentHeight) * texScale;
            v2.uv = glm::vec2(uvX1, 0) * texScale;
            v3.uv = glm::vec2(uvX2, 0) * texScale;
            v4.uv = glm::vec2(uvX2, segmentHeight) * texScale;
            SetNormalsAndTangentsFromVertices(&v3, &v2, &v1);
            SetNormalsAndTangentsFromVertices(&v3, &v1, &v4);
            vertices.push_back(v3);
            vertices.push_back(v2);
            vertices.push_back(v1);
            vertices.push_back(v3);
            vertices.push_back(v1);
            vertices.push_back(v4);

            if (hasTrims) {
                Transform trimTransform;
                trimTransform.position = cursor;
                trimTransform.rotation.y = Util::YRotationBetweenTwoPoints(v4.position, v1.position) + HELL_PI;
                trimTransform.scale.x = segmentWidth * WALL_HEIGHT;
                ceilingTrims.push_back(trimTransform);
                floorTrims.push_back(trimTransform);
            }

            // Bit above the window
            Vertex v5, v6, v7, v8;
            v5.position = intersectionPoint + glm::vec3(0, WINDOW_HEIGHT, 0);
            v6.position = intersectionPoint + glm::vec3(0, height, 0);
            v7.position = intersectionPoint + (wallDir * (WINDOW_WIDTH + 0.005f)) + glm::vec3(0, height, 0);
            v8.position = intersectionPoint + (wallDir * (WINDOW_WIDTH + 0.005f)) + glm::vec3(0, WINDOW_HEIGHT, 0);
            segmentWidth = abs(glm::length((v8.position - v5.position))) / WALL_HEIGHT;
            segmentHeight = glm::length((v6.position - v5.position)) / WALL_HEIGHT;
            uvX1 = uvX2;
            uvX2 = uvX1 + segmentWidth;
            v5.uv = glm::vec2(uvX1, segmentHeight) * texScale;
            v6.uv = glm::vec2(uvX1, 0) * texScale;
            v7.uv = glm::vec2(uvX2, 0) * texScale;
            v8.uv = glm::vec2(uvX2, segmentHeight) * texScale;
            SetNormalsAndTangentsFromVertices(&v7, &v6, &v5);
            SetNormalsAndTangentsFromVertices(&v7, &v5, &v8);
            vertices.push_back(v7);
            vertices.push_back(v6);
            vertices.push_back(v5);
            vertices.push_back(v7);
            vertices.push_back(v5);
            vertices.push_back(v8);

            // Bit below the window
            {
                float windowYBegin = 0.8f;
                float height = windowYBegin + 0.1f;

                Vertex v5, v6, v7, v8;
                v5.position = intersectionPoint + glm::vec3(0, 0, 0);
                v6.position = intersectionPoint + glm::vec3(0, height, 0);
                v7.position = intersectionPoint + (wallDir * (WINDOW_WIDTH + 0.005f)) + glm::vec3(0, height, 0);
                v8.position = intersectionPoint + (wallDir * (WINDOW_WIDTH + 0.005f)) + glm::vec3(0, 0, 0);
                segmentWidth = abs(glm::length((v8.position - v5.position))) / WALL_HEIGHT;
                segmentHeight = glm::length((v6.position - v5.position)) / WALL_HEIGHT;
                uvX1 = uvX2;
                uvX2 = uvX1 + segmentWidth;
                v5.uv = glm::vec2(uvX1, segmentHeight) * texScale;
                v6.uv = glm::vec2(uvX1, 0) * texScale;
                v7.uv = glm::vec2(uvX2, 0) * texScale;
                v8.uv = glm::vec2(uvX2, segmentHeight) * texScale;
                SetNormalsAndTangentsFromVertices(&v7, &v6, &v5);
                SetNormalsAndTangentsFromVertices(&v7, &v5, &v8);
                vertices.push_back(v7);
                vertices.push_back(v6);
                vertices.push_back(v5);
                vertices.push_back(v7);
                vertices.push_back(v5);
                vertices.push_back(v8);
            }

            if (hasTrims) {
                Transform trimTransform;
                trimTransform.position = intersectionPoint;
                trimTransform.rotation.y = Util::YRotationBetweenTwoPoints(v4.position, v1.position) + HELL_PI;
                trimTransform.scale.x = segmentWidth * WALL_HEIGHT;
                ceilingTrims.push_back(trimTransform);
                floorTrims.push_back(trimTransform);
            }


            cursor = intersectionPoint + (wallDir * (WINDOW_WIDTH + 0.005f)); // This 0.05 is so you don't get an intersection with the door itself
            uvX1 = uvX2;
        }

        // You're on the final bit of wall then aren't ya
        else {

            // The wall piece from cursor to door
            Vertex v1, v2, v3, v4;
            v1.position = cursor;
            v2.position = cursor + glm::vec3(0, height, 0);
            v3.position = wallEnd + glm::vec3(0, height, 0);
            v4.position = wallEnd;
            float segmentWidth = abs(glm::length((v4.position - v1.position))) / WALL_HEIGHT;
            float segmentHeight = glm::length((v2.position - v1.position)) / WALL_HEIGHT;
            uvX2 = uvX1 + segmentWidth;
            v1.uv = glm::vec2(uvX1, segmentHeight) * texScale;
            v2.uv = glm::vec2(uvX1, 0) * texScale;
            v3.uv = glm::vec2(uvX2, 0) * texScale;
            v4.uv = glm::vec2(uvX2, segmentHeight) * texScale;

            SetNormalsAndTangentsFromVertices(&v3, &v2, &v1);
            SetNormalsAndTangentsFromVertices(&v3, &v1, &v4);
            vertices.push_back(v3);
            vertices.push_back(v2);
            vertices.push_back(v1);
            vertices.push_back(v3);
            vertices.push_back(v1);
            vertices.push_back(v4);
            finishedBuildingWall = true;

            if (hasTrims) {
                Transform trimTransform;
                trimTransform.position = cursor;
                trimTransform.rotation.y = Util::YRotationBetweenTwoPoints(v4.position, v1.position) + HELL_PI;
                trimTransform.scale.x = segmentWidth * WALL_HEIGHT;
                ceilingTrims.push_back(trimTransform);
                floorTrims.push_back(trimTransform);
            }

            Line collisionLine;
            collisionLine.p1.pos = v1.position;
            collisionLine.p2.pos = v4.position;
            collisionLine.p1.color = YELLOW;
            collisionLine.p2.color = RED;
            collisionLines.push_back(collisionLine);
        }
    }
    // Create indices
    indices.clear();
    for (uint32_t i = 0; i < vertices.size(); i++) {
        indices.push_back(i);
    }

    // Create AABB
    glm::vec3 boundsMin = glm::vec3(1e30f);
    glm::vec3 boundsMax = glm::vec3(-1e30f);
    for (int i = 0; i < indices.size(); i++) {
        Vertex* vertex = &vertices[indices[i]];
        boundsMin = Util::Vec3Min(boundsMin, vertex->position);
        boundsMax = Util::Vec3Max(boundsMax, vertex->position);
    }
    aabb = AABB(boundsMin, boundsMax);
}

void Wall::CreateMeshGL() {
    if (VAO != 0) {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
    }
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));
}

glm::vec3 Wall::GetNormal() {
    glm::vec3 vector = glm::normalize(begin - end);
    return glm::vec3(-vector.z, 0, vector.x) * glm::vec3(-1);
}

glm::vec3 Wall::GetMidPoint() {
    return (begin + end) * glm::vec3(0.5);
}

void Wall::Draw() {
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, vertices.size());
}

void Wall::UpdateRenderItems() {

    renderItems.clear();

    Mesh* mesh = AssetManager::GetMeshByIndex(meshIndex);
    RenderItem3D renderItem;
    renderItem.vertexOffset = mesh->baseVertex;
    renderItem.indexOffset = mesh->baseIndex;
    renderItem.modelMatrix = glm::mat4(1);
    renderItem.inverseModelMatrix = inverse(renderItem.modelMatrix);
    renderItem.meshIndex = meshIndex;
    renderItem.baseColorTextureIndex = AssetManager::GetMaterialByIndex(materialIndex)->_basecolor;
    renderItem.normalTextureIndex = AssetManager::GetMaterialByIndex(materialIndex)->_normal;
    renderItem.rmaTextureIndex = AssetManager::GetMaterialByIndex(materialIndex)->_rma;
    renderItems.push_back(renderItem);

    static int ceilingTrimMeshIndex = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("TrimCeiling"))->GetMeshIndices()[0];
    static int floorTrimMeshIndex = AssetManager::GetModelByIndex(AssetManager::GetModelIndexByName("TrimFloor"))->GetMeshIndices()[0];

    for (Transform& transform : ceilingTrims) {
        RenderItem3D renderItem;
        renderItem.vertexOffset = AssetManager::GetMeshByIndex(ceilingTrimMeshIndex)->baseVertex;
        renderItem.indexOffset = AssetManager::GetMeshByIndex(ceilingTrimMeshIndex)->baseIndex;
        renderItem.modelMatrix = transform.to_mat4();
        renderItem.inverseModelMatrix = inverse(renderItem.modelMatrix);
        renderItem.meshIndex = ceilingTrimMeshIndex;
        renderItem.baseColorTextureIndex = AssetManager::GetMaterialByIndex(materialIndex)->_basecolor;
        renderItem.normalTextureIndex = AssetManager::GetMaterialByIndex(materialIndex)->_normal;
        renderItem.rmaTextureIndex = AssetManager::GetMaterialByIndex(materialIndex)->_rma;
        renderItems.push_back(renderItem);
    }

    for (Transform& transform : floorTrims) {
        RenderItem3D renderItem;
        renderItem.vertexOffset = AssetManager::GetMeshByIndex(floorTrimMeshIndex)->baseVertex;
        renderItem.indexOffset = AssetManager::GetMeshByIndex(floorTrimMeshIndex)->baseIndex;
        renderItem.modelMatrix = transform.to_mat4();
        renderItem.inverseModelMatrix = inverse(renderItem.modelMatrix);
        renderItem.meshIndex = floorTrimMeshIndex;
        renderItem.baseColorTextureIndex = AssetManager::GetMaterialByIndex(materialIndex)->_basecolor;
        renderItem.normalTextureIndex = AssetManager::GetMaterialByIndex(materialIndex)->_normal;
        renderItem.rmaTextureIndex = AssetManager::GetMaterialByIndex(materialIndex)->_rma;
        renderItems.push_back(renderItem);
    }
}

std::vector<RenderItem3D>& Wall::GetRenderItems() {
    return renderItems;
}