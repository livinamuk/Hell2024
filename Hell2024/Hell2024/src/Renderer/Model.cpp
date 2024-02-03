#include <limits>
#include <numeric>
#include <algorithm>
#include <unordered_map>

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include "Model.h"
#include "../Util.hpp"

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


void Model::Load(std::string filepath, const bool bake_on_load) {

	_filename = filepath.substr(filepath.rfind("/") + 1);
	_name = _filename.substr(0, _filename.length() - 4);

	//std::cout << "makign new model with name " << _name << "\n";

	if (!Util::FileExists(filepath.c_str()))
		std::cout << filepath << " does not exist!\n";

	Assimp::Importer importer;

	constexpr uint32_t flags{
		aiProcess_LimitBoneWeights |
		aiProcess_Triangulate |
		aiProcess_GenSmoothNormals |
		aiProcess_FlipUVs |
		aiProcess_CalcTangentSpace
	};
	const auto scene{ importer.ReadFile(filepath, flags) };

	_meshes.reserve(scene->mNumMeshes);
	glm::vec3 min_pos{ std::numeric_limits<float>::max() };
	glm::vec3 max_pos{ std::numeric_limits<float>::min() };

	static const auto sum_of_indices{
		[](const auto total, const auto &f) { return total + f.mNumIndices; } };

	static const auto count_indices{ [](const auto mesh) {
		return mesh->mNumFaces +
			std::accumulate(mesh->mFaces, mesh->mFaces + mesh->mNumFaces, 0, sum_of_indices);
	} };

	const aiVector3D empty_vec{};
	const glm::vec3 empty{};

	for (size_t mesh_index{}; mesh_index < scene->mNumMeshes; ++mesh_index) {
		const auto mesh{ scene->mMeshes[mesh_index] };

		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		vertices.reserve(mesh->mNumVertices);
		indices.reserve(count_indices(mesh));

		for (size_t vertex_index{}; vertex_index < mesh->mNumVertices; ++vertex_index) {
			const auto &v{ mesh->mVertices[vertex_index] };
			const auto &n{ mesh->mNormals[vertex_index] };
			const auto &uv{ mesh->mTextureCoords[0] != nullptr ? mesh->mTextureCoords[0][vertex_index] : empty_vec };
			const auto &t{ mesh->mTangents != nullptr ? mesh->mTangents[vertex_index] : empty_vec };
			const auto &bt{ mesh->mBitangents != nullptr ? mesh->mBitangents[vertex_index] : empty_vec };

			// store bounding box shit
			min_pos.x = std::min(min_pos.x, v.x);
			min_pos.y = std::min(min_pos.y, v.y);
			min_pos.z = std::min(min_pos.z, v.z);
			max_pos.x = std::max(max_pos.x, v.x);
			max_pos.y = std::max(max_pos.y, v.y);
			max_pos.z = std::max(max_pos.z, v.z);

			vertices.emplace_back(
				glm::vec3{ v.x, v.y, v.z },    // position
				glm::vec3{ n.x, n.y, n.z },    // normal
				glm::vec2{ uv.x, uv.y },       // uv
				glm::vec3{ t.x, t.y, t.z },    // tangent
				glm::vec3{ bt.x, bt.y, bt.z }, // bitangent
				glm::vec4{ 0.0f },             // weight
				glm::ivec4{ 0 }                // boneID
			);
		}

		for (size_t face_index{}; face_index < mesh->mNumFaces; ++face_index) {
			const auto &face{ mesh->mFaces[face_index] };
			for (size_t i{}; i < face.mNumIndices; ++i) {
				indices.emplace_back(face.mIndices[i]);
			}
		}

		if (indices.size() % 3 != 0) {
			_meshes.emplace_back( std::move(vertices), std::move(indices),
				std::string{ mesh->mName.C_Str() }, bake_on_load);
			continue;
		}

		// Ensure the tangents and bitangents are computed
		for (size_t i{}; i < indices.size(); i += 3) {
			auto &v0{ vertices[indices[i]] };
			if (v0.tangent == empty) continue;

			auto &v1{ vertices[indices[i + 1]] };
			auto &v2{ vertices[indices[i + 2]] };

			const auto delta_pos_1{ v1.position - v0.position };
			const auto delta_pos_2{ v2.position - v0.position };
			const auto delta_uv_1{ v1.uv - v0.uv };
			const auto delta_uv_2{ v2.uv - v0.uv };

			const float r{ 1.0f / (delta_uv_1.x * delta_uv_2.y - delta_uv_1.y * delta_uv_2.x) };
			const auto tangent{ (delta_pos_1 * delta_uv_2.y - delta_pos_2 * delta_uv_1.y) * r };
			const auto bitangent{ (delta_pos_2 * delta_uv_1.x - delta_pos_1 * delta_uv_2.x) * r };

			v0.tangent = tangent;
			v1.tangent = tangent;
			v2.tangent = tangent;

			v0.bitangent = bitangent;
			v1.bitangent = bitangent;
			v2.bitangent = bitangent;
		}

		_meshes.emplace_back(std::move(vertices), std::move(indices),
			std::string{ mesh->mName.C_Str() }, bake_on_load);
	}

	// Build the bounding box
	_boundingBox.size = max_pos - min_pos;
	_boundingBox.offsetFromModelOrigin = min_pos;
}

void Model::Draw() {
	for (int i = 0; i < _meshes.size(); ++i)
		_meshes[i].Draw();;
}

void Model::CreateTriangleMesh() {
	std::vector<PxVec3> pxvertices;
	std::vector<unsigned int> pxindices;
	int baseIndex = 0;
	for (Mesh& mesh : _meshes) {
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

void Model::Bake() {
	for (auto &mesh : _meshes) {
		mesh.Bake();
	}
	_baked = true;
}

bool Model::IsBaked() {
	return _baked;
}
