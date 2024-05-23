#include "Floor.h"
#include "../../Core/AssetManager.h"
#include "../../Util.hpp"

///////////////
//           // 
//   Floor   //

Floor::Floor(float x1, float z1, float x2, float z2, float height, int materialIndex, float textureScale) {
    this->materialIndex = materialIndex;
    this->textureScale = textureScale;

    v1.position = glm::vec3(x1, height, z1);
    v2.position = glm::vec3(x1, height, z2);
    v3.position = glm::vec3(x2, height, z2);
    v4.position = glm::vec3(x2, height, z1);

    // remove these. you are only using them to create the bathroom ceiling. 
    this->x1 = x1;
    this->z1 = z1;
    this->x2 = x2;
    this->z2 = z2;
    this->height = height;
}

Floor::Floor(glm::vec3 pos1, glm::vec3 pos2, glm::vec3 pos3, glm::vec3 pos4, int materialIndex, float textureScale) {
    this->materialIndex = materialIndex;
    this->textureScale = textureScale;
    v1.position = pos1;
    v2.position = pos2;
    v3.position = pos3;
    v4.position = pos4;
}

void Floor::Draw() {
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, vertices.size());
}

void Floor::CreateMeshGL() {
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


bool Floor::PointIsAboveThisFloor(glm::vec3 point) {
    glm::vec2 p = glm::vec2(point.x, point.z);
    glm::vec2 vert1 = glm::vec2(v1.position.x, v1.position.z);
    glm::vec2 vert2 = glm::vec2(v2.position.x, v2.position.z);
    glm::vec2 vert3 = glm::vec2(v3.position.x, v3.position.z);
    glm::vec2 vert4 = glm::vec2(v4.position.x, v4.position.z);
    return (Util::PointIn2DTriangle(p, vert1, vert2, vert3) || Util::PointIn2DTriangle(p, vert1, vert3, vert4));
}

void Floor::CreateVertexData() {

    vertices.clear();
    indices.clear();

    glm::vec3 normal = Util::NormalFromTriangle(v1.position, v2.position, v3.position);
    v1.normal = normal;
    v2.normal = normal;
    v3.normal = normal;
    v4.normal = normal;
    v1.uv = glm::vec2(v1.position.x, v1.position.z) / textureScale;
    v2.uv = glm::vec2(v2.position.x, v2.position.z) / textureScale;
    v3.uv = glm::vec2(v3.position.x, v3.position.z) / textureScale;
    v4.uv = glm::vec2(v4.position.x, v4.position.z) / textureScale;
    Util::SetNormalsAndTangentsFromVertices(&v1, &v2, &v3);
    Util::SetNormalsAndTangentsFromVertices(&v3, &v4, &v1);
    vertices.clear();
    vertices.push_back(v1);
    vertices.push_back(v2);
    vertices.push_back(v3);
    vertices.push_back(v3);
    vertices.push_back(v4);
    vertices.push_back(v1);

    indices = { 0, 1, 2, 3, 4, 5 };

    // Create AABB
    glm::vec3 boundsMin = glm::vec3(1e30f);
    glm::vec3 boundsMax = glm::vec3(-1e30f);
    for (int i = 0; i < indices.size(); i++) {
        Vertex* vertex = &vertices[indices[i]];
        boundsMin = fminf(boundsMin, vertex->position);
        boundsMax = fmaxf(boundsMax, vertex->position);
    }
    aabb = AABB(boundsMin, boundsMax);
}

void Floor::UpdateRenderItem() {
    Mesh* mesh = AssetManager::GetMeshByIndex(meshIndex);
    renderItem.vertexOffset = mesh->baseVertex;
    renderItem.indexOffset = mesh->baseIndex;
    renderItem.modelMatrix = glm::mat4(1);
    renderItem.inverseModelMatrix = inverse(renderItem.modelMatrix);
    renderItem.meshIndex = meshIndex;
    renderItem.baseColorTextureIndex = AssetManager::GetMaterialByIndex(materialIndex)->_basecolor;
    renderItem.normalTextureIndex = AssetManager::GetMaterialByIndex(materialIndex)->_normal;
    renderItem.rmaTextureIndex = AssetManager::GetMaterialByIndex(materialIndex)->_rma;
}

RenderItem3D& Floor::GetRenderItem() {
    return renderItem;
}