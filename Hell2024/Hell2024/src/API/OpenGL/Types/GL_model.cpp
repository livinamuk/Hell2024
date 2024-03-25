#include "GL_model.h"
#include <unordered_map>
#include "../../../Core/AssetManager.h"
#include "../../../Common.h"
#include "../../../Util.hpp"

inline glm::vec3 NormalFromThreePoints(glm::vec3 pos0, glm::vec3 pos1, glm::vec3 pos2) {
	return glm::normalize(glm::cross(pos1 - pos0, pos2 - pos0));
}

inline void SetNormalsAndTangentsFromVertices(Vertex* vert0, Vertex* vert1, Vertex* vert2) {
	// Shortcuts for UVs
	glm::vec3& v0 = vert0->position;
	glm::vec3& v1 = vert1->position;
	glm::vec3& v2 = vert2->position;
	glm::vec2& uv0 = vert0->uv;
	glm::vec2& uv1 = vert1->uv;
	glm::vec2& uv2 = vert2->uv;
	// Edges of the triangle : position delta. UV delta
	glm::vec3 deltaPos1 = v1 - v0;
	glm::vec3 deltaPos2 = v2 - v0;
	glm::vec2 deltaUV1 = uv1 - uv0;
	glm::vec2 deltaUV2 = uv2 - uv0;
	float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
	glm::vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
	glm::vec3 bitangent = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r;
	glm::vec3 normal = NormalFromThreePoints(vert0->position, vert1->position, vert2->position);
	vert0->normal = normal;
	vert1->normal = normal;
	vert2->normal = normal;
	vert0->tangent = tangent;
	vert1->tangent = tangent;
	vert2->tangent = tangent;
	vert0->bitangent = bitangent;
	vert1->bitangent = bitangent;
	vert2->bitangent = bitangent;
}

inline void TangentFromUVs(Vertex* v0, Vertex* v1, Vertex* v2) {
	glm::vec3 deltaPos1 = v1->position - v0->position;
	glm::vec3 deltaPos2 = v2->position - v0->position;
	glm::vec2 deltaUV1 = v1->uv - v0->uv;
	glm::vec2 deltaUV2 = v2->uv - v0->uv;
	float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
	glm::vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
	v0->tangent = tangent;
	v1->tangent = tangent;
	v2->tangent = tangent;
}

void OpenGLModel::Draw() {
    for (int i = 0; i < _meshes.size(); ++i)
        _meshes[i].Draw();;
}

void OpenGLModel::CreateTriangleMesh() {
    std::vector<PxVec3> pxvertices;
    std::vector<unsigned int> pxindices;
    int baseIndex = 0;
    for (OpenGLMesh& mesh : _meshes) {
        for (auto& vertex : mesh.vertices) {
            pxvertices.push_back(PxVec3(vertex.position.x, vertex.position.y, vertex.position.z));
        }
        for (auto& index : mesh.indices) {
            pxindices.push_back(index + baseIndex);
        }
        baseIndex = pxvertices.size();
    }
    if (pxindices.size()) {

        _triangleMesh = Physics::CreateTriangleMesh(pxvertices.size(), pxvertices.data(), pxindices.size() / 3, pxindices.data());
        //std::cout << "Created triangle mesh for model " << _name << " out of " << _meshes.size() << " mesh\n";
    }
}

void OpenGLModel::Bake() {
    for (auto& mesh : _meshes) {
        mesh.Bake();
        AssetManager::CreateMesh(mesh._name, mesh.vertices, mesh.indices);
    }
    _baked = true;
}

bool OpenGLModel::IsBaked() {
    return _baked;
}