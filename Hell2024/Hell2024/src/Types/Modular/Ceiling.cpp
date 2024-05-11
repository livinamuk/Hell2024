#include "Ceiling.h"
#include "../../Core/AssetManager.h"
#include "../../Util.hpp"

Ceiling::Ceiling(float x1, float z1, float x2, float z2, float height, int materialIndex) {
    this->materialIndex = materialIndex;
    this->x1 = x1;
    this->z1 = z1;
    this->x2 = x2;
    this->z2 = z2;
    this->height = height;
    

    if (VAO != 0) {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
    }
}

void Ceiling::Draw() {
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, vertices.size());
}

void Ceiling::CreateMeshGL() {
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

void Ceiling::CreateVertexData() {

    vertices.clear();

    Vertex v1, v2, v3, v4;
    v1.position = glm::vec3(x1, height, z1);
    v2.position = glm::vec3(x1, height, z2);
    v3.position = glm::vec3(x2, height, z2);
    v4.position = glm::vec3(x2, height, z1);
    glm::vec3 normal = Util::NormalFromTriangle(v3.position, v2.position, v1.position);
    v1.normal = normal;
    v2.normal = normal;
    v3.normal = normal;
    v4.normal = normal;
    float scale = 2.0f;
    float uv_x_low = z1 / scale;
    float uv_x_high = z2 / scale;
    float uv_y_low = x1 / scale;
    float uv_y_high = x2 / scale;
    uv_x_low = x1;
    uv_x_high = x2;
    uv_y_low = z1;
    uv_y_high = z2;
    v1.uv = glm::vec2(uv_x_low, uv_y_low);
    v2.uv = glm::vec2(uv_x_low, uv_y_high);
    v3.uv = glm::vec2(uv_x_high, uv_y_high);
    v4.uv = glm::vec2(uv_x_high, uv_y_low);
    Util::SetNormalsAndTangentsFromVertices(&v3, &v2, &v1);
    Util::SetNormalsAndTangentsFromVertices(&v1, &v4, &v3);
    vertices.push_back(v3);
    vertices.push_back(v2);
    vertices.push_back(v1);

    vertices.push_back(v1);
    vertices.push_back(v4);
    vertices.push_back(v3);
    
    indices.clear();
    indices = { 0, 1, 2, 3, 4, 5 };
}

void Ceiling::UpdateRenderItem() {
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

RenderItem3D& Ceiling::GetRenderItem() {
    return renderItem;
}