#pragma once
#include "HeightMap.h"
#include <stb_image.h>
#include <iostream>

void HeightMap::Load(const std::string& filepath, float textureRepeat) {
    int channels;
    unsigned char* data = stbi_load(filepath.c_str(), &m_width, &m_depth, &channels, 0);
    if (!data) {
        std::cout << "Failed to load heightmap image\n";
        return;
    }

    // Vertices
    m_vertices.reserve(m_width * m_depth);
    for (int z = 0; z < m_depth; z ++) {
        for (int x = 0; x < m_width; x++) { 
            int index = (z * m_width + x) * channels;
            unsigned char value = data[index];
            float heightValue = static_cast<float>(value) / 255.0f;
            Vertex& vertex = m_vertices.emplace_back();
            vertex.position = glm::vec3(x, heightValue, z);
            float textureRepeat = 1000.0f;
            vertex.uv = glm::vec2((float)x / (m_width - 1) * textureRepeat, float(z) / (m_depth - 1) * textureRepeat);
        }
    }
    stbi_image_free(data);

    // Normals
    for (int z = 0; z < m_depth; z++) {
        for (int x = 0; x < m_width; x++) {
            glm::vec3 current = m_vertices[z * m_width + x].position;
            glm::vec3 left = (x > 0) ? m_vertices[z * m_width + (x - 1)].position : current;
            glm::vec3 right = (x < m_width - 1) ? m_vertices[z * m_width + (x + 1)].position : current;
            glm::vec3 down = (z > 0) ? m_vertices[(z - 1) * m_width + x].position : current;
            glm::vec3 up = (z < m_depth - 1) ? m_vertices[(z + 1) * m_width + x].position : current;
            glm::vec3 dx = right - left;
            glm::vec3 dz = up - down;
            glm::vec3 normal = glm::normalize(glm::cross(dz, dx));
            m_vertices[z * m_width + x].normal = normal;
        }
    }
    // Indices
    m_indices.reserve((m_width - 1) * (m_depth - 1) * 2 * 3); // Rough estimate of the size
    for (int z = 0; z < m_depth - 1; ++z) {
        for (int x = 0; x < m_width; ++x) {
            m_indices.push_back(z * m_width + x);
            m_indices.push_back((z + 1) * m_width + x);
        }
        // Degenerate triangles to connect rows
        if (z < m_depth - 2) {
            m_indices.push_back((z + 1) * m_width + (m_width - 1));
            m_indices.push_back((z + 1) * m_width);
        }
    }
    std::cout << "Heightmap loaded: " << m_vertices.size() << " vertices\n";
}

void HeightMap::UploadToGPU() {
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_EBO);
    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(Vertex), &m_vertices[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(unsigned int), &m_indices[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
    glBindVertexArray(0);
}

void HeightMap::CreatePhysicsObject() {

    //PxShapeFlags shapeFlags(PxShapeFlag::eSCENE_QUERY_SHAPE); // Most importantly NOT eSIMULATION_SHAPE. PhysX does not allow for tri mesh.
    PxShapeFlags shapeFlags(PxShapeFlag::eSCENE_QUERY_SHAPE | PxShapeFlag::eSIMULATION_SHAPE);

    PxHeightField* m_pxHeightField = Physics::CreateHeightField(m_vertices, m_width, m_depth);

    float heightScale = 1.0f * m_transform.scale.y;
    float rowScale = m_transform.scale.x;
    float colScale = m_transform.scale.z;

    PxShape* m_pxShape = Physics::CreateShapeFromHeightField(m_pxHeightField, shapeFlags, heightScale, rowScale, colScale);

    PhysicsFilterData filterData;
    filterData.raycastGroup = RAYCAST_ENABLED;
    filterData.collisionGroup = ENVIROMENT_OBSTACLE;
    filterData.collidesWith = (CollisionGroup)(GENERIC_BOUNCEABLE | BULLET_CASING | PLAYER | RAGDOLL);

    m_pxRigidStatic = Physics::CreateRigidStatic(m_transform, filterData, m_pxShape);
    m_pxRigidStatic->userData = new PhysicsObjectData(PhysicsObjectType::HEIGHT_MAP, this);

}