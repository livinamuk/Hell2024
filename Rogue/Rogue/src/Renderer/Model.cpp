#include "Model.h"
#include "../Util.hpp"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#include <unordered_map>

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
	// Edges of the triangle : postion delta. UV delta
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

	if (!Util::FileExists(filepath.c_str()))
		std::cout << filepath << " does not exist!\n";

	constexpr size_t batch_size{ 128 };
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	shapes.reserve(batch_size);
	materials.reserve(batch_size);
	std::string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str())) {
		std::cout << "Crashed loading " << filepath.c_str() << "\n";
		return;
	}

	std::unordered_map<Vertex, uint32_t> uniqueVertices = {};

	glm::vec3 minPos = glm::vec3(9999, 9999, 9999);
	glm::vec3 maxPos = glm::vec3(0, 0, 0);

	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	vertices.reserve(batch_size);
	indices.reserve(batch_size);

	// Loop over shapes
	for (size_t s = 0; s < shapes.size(); s++) {
		// Loop over faces(polygon)
		size_t index_offset = 0;
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
			size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);

			// Loop over vertices in the face.
			for (size_t v = 0; v < fv; v++)
			{
				Vertex vertex;

				// access to vertex
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
				vertex.position.x = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
				vertex.position.y = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
				vertex.position.z = attrib.vertices[3 * size_t(idx.vertex_index) + 2];
				if (idx.normal_index >= 0) {
					vertex.normal.x = attrib.normals[3 * size_t(idx.normal_index) + 0];
					vertex.normal.y = attrib.normals[3 * size_t(idx.normal_index) + 1];
					vertex.normal.z = attrib.normals[3 * size_t(idx.normal_index) + 2];
				}
				// Check if `texcoord_index` is zero or positive. negative = no texcoord data
				if (idx.texcoord_index >= 0) {
					vertex.uv.x = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
					vertex.uv.y = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];
				}
				vertices.emplace_back(std::move(vertex));
				indices.push_back(indices.size());
			}
			index_offset += fv;

			// per-face material
			shapes[s].mesh.material_ids[f];
		}
		//	Mesh* mesh = new Mesh(vertices, indices, shapes[s].name);
		//	this->m_meshes.push_back(mesh);
	}



	for (const auto& shape : shapes)
	{
		std::vector<Vertex> vertices;
		std::vector<unsigned int> indices;
		vertices.reserve(shape.mesh.indices.size());
		indices.reserve(shape.mesh.indices.size());

		for (const auto &index : shape.mesh.indices) {
			//		for (const auto& index : shape.mesh.indices) {
			Vertex vertex = {};
			//vertex.MaterialID = shape.mesh.material_ids[i / 3];

			vertex.position = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			// Check if `normal_index` is zero or positive. negative = no normal data
			if (index.normal_index >= 0) {
				vertex.normal.x = attrib.normals[3 * size_t(index.normal_index) + 0];
				vertex.normal.y = attrib.normals[3 * size_t(index.normal_index) + 1];
				vertex.normal.z = attrib.normals[3 * size_t(index.normal_index) + 2];
			}
			// store bounding box shit
			minPos.x = std::min(minPos.x, vertex.position.x);
			minPos.y = std::min(minPos.y, vertex.position.y);
			minPos.z = std::min(minPos.z, vertex.position.z);
			maxPos.x = std::max(maxPos.x, vertex.position.x);
			maxPos.y = std::max(maxPos.y, vertex.position.y);
			maxPos.z = std::max(maxPos.z, vertex.position.z);

			if (attrib.texcoords.size() && index.texcoord_index != -1) { // should only be 1 or 2, some bug in debug where there were over 1000 on the spherelines model...
				//m_hasTexCoords = true;
				vertex.uv = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
				};
			}

			if (uniqueVertices.count(vertex) == 0) {
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}

			indices.push_back(uniqueVertices[vertex]);
		}

		// for (int i = 0; i < indices.size(); i += 3) {
			// SetNormalsAndTangentsFromVertices(&vertices[indices[i]], &vertices[indices[i + 1]], &vertices[indices[i + 2]]);
		// }

		for (int i = 0; i < indices.size(); i += 3) {
			Vertex* vert0 = &vertices[indices[i]];
			Vertex* vert1 = &vertices[indices[i + 1]];
			Vertex* vert2 = &vertices[indices[i + 2]];
			// Shortcuts for UVs
			glm::vec3& v0 = vert0->position;
			glm::vec3& v1 = vert1->position;
			glm::vec3& v2 = vert2->position;
			glm::vec2& uv0 = vert0->uv;
			glm::vec2& uv1 = vert1->uv;
			glm::vec2& uv2 = vert2->uv;
			// Edges of the triangle : postion delta. UV delta
			glm::vec3 deltaPos1 = v1 - v0;
			glm::vec3 deltaPos2 = v2 - v0;
			glm::vec2 deltaUV1 = uv1 - uv0;
			glm::vec2 deltaUV2 = uv2 - uv0;
			float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
			glm::vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
			glm::vec3 bitangent = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r;
			vert0->tangent = tangent;
			vert1->tangent = tangent;
			vert2->tangent = tangent;
			vert0->bitangent = bitangent;
			vert1->bitangent = bitangent;
			vert2->bitangent = bitangent;
			//std::cout << i << Util::Vec3ToString(bitangent) << "\n";
		}
		_meshes.emplace_back(std::move(vertices), std::move(indices),
			std::string(shape.name), bake_on_load);
	}

	// Build the bounding box
	float width = std::abs(maxPos.x - minPos.x);
	float height = std::abs(maxPos.y - minPos.y);
	float depth = std::abs(maxPos.z - minPos.z);
	_boundingBox.size = glm::vec3(width, height, depth);
	_boundingBox.offsetFromModelOrigin = minPos;


	// Get the final missing pieces for the bounding box
	//m_boundingBox.scale = (minPos * glm::vec3(-1)) + maxPos;
	//m_boundingBox.offsetFromModelOrigin = (m_boundingBox.scale * glm::vec3(0.5)) + minPos;

	// Organise triangles (for raycasting)
	/*for (int m = 0; m < _meshes.size(); m++) {
		Mesh* mesh = m_meshes[m];;
		for (size_t i = 0; i < mesh->indices.size(); i += 3) {
			size_t i0 = mesh->indices[i];
			size_t i1 = mesh->indices[i + 1];
			size_t i2 = mesh->indices[i + 2];

			Triangle tri;
			tri.p1 = mesh->vertices[i0].Position;
			tri.p2 = mesh->vertices[i1].Position;
			tri.p3 = mesh->vertices[i2].Position;
			_triangles.push_back(tri);
		}
	}*/

	for (int m = 0; m < _meshes.size(); m++) {
	//	std::cout << m << ": " << _meshes[m]._name << " with " << _meshes[m].vertices.size() << " vertices\n";
	}
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
}
