#pragma once
#include "../Common.h"
#include <vector>
#include "../Core/Physics.h"

class Mesh {

	public:
		Mesh();
		Mesh (std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::string name, const bool instant_bake = true);
		void Draw();
		int GetIndexCount();
		int GetVAO();
		void CreateTriangleMesh();
		void CreateConvexMesh();
		void Bake();

		unsigned int _baseVertex = 0;
		unsigned int _indexCount = 0;

	public:
		unsigned int _VBO = 0;
		unsigned int _VAO = 0;
		unsigned int _EBO = 0;
		std::string _name;
		std::vector<Vertex> vertices;
		std::vector<unsigned int> indices;
		PxTriangleMesh* _triangleMesh = NULL;
		PxConvexMesh* _convexMesh = NULL;
};