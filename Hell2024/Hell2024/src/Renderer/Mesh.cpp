#include "Mesh.h"

Mesh::Mesh() {
}

Mesh::Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::string name, const bool instant_bake)
    : _name{ std::move(name) }
    , vertices{ std::move(vertices) }
    , indices{ std::move(indices) }
{
    if (instant_bake) Bake();
}

void Mesh::Draw() {
    glBindVertexArray(_VAO);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
}

int Mesh::GetIndexCount() {
    return static_cast<int>(indices.size());
}

int Mesh::GetVAO() {
    return _VAO;
}

void Mesh::CreateTriangleMesh() {
    std::vector<PxVec3> pxvertices;
    std::vector<unsigned int> pxindices;
    for (auto& vertex : vertices) {
        pxvertices.push_back(PxVec3(vertex.position.x, vertex.position.y, vertex.position.z));
    }
    for (auto& index : indices) {
        pxindices.push_back(index);
    }
    if (pxindices.size()) {
        _triangleMesh = Physics::CreateTriangleMesh(pxvertices.size(), pxvertices.data(), pxindices.size() / 3, pxindices.data());
        //std::cout << "Created triangle mesh for " << _name << "\n";
    }
}

void Mesh::CreateConvexMesh() {
    std::vector<PxVec3> pxvertices;
    for (auto& vertex : vertices) {
        bool found = false;
        for  (auto& uniqueVertex : pxvertices) {

            if (vertex.position.x == uniqueVertex.x && vertex.position.y == uniqueVertex.y && vertex.position.z == uniqueVertex.z) {
                found = true;
                break;
            }
        }
        if (!found) {
            pxvertices.push_back(PxVec3(vertex.position.x, vertex.position.y, vertex.position.z));
        }
    }
    if (pxvertices.size()) {
        _convexMesh = Physics::CreateConvexMesh(pxvertices.size(), pxvertices.data());
        //std::cout << "Created convex mesh for " << _name << " with " << pxvertices.size() << " vertices\n";
    }
}

void Mesh::Bake() {
    glGenVertexArrays(1, &_VAO);
    glGenBuffers(1, &_VBO);
    glGenBuffers(1, &_EBO);

    glBindVertexArray(_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, _VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, bitangent));

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}
