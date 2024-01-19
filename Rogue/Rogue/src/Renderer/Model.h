#pragma once
#include "../Common.h"
#include "Mesh.h"
#include "../Core/Physics.h"

struct BoundingBox {
	glm::vec3 size;
	glm::vec3 offsetFromModelOrigin;
};

class Model
{
public:
	void Load(std::string filepath, const bool bake_on_load = true);
	void Draw();
	void CreateTriangleMesh();
	//void Draw(Shader* shader, glm::mat4 modelMatrix, int primitiveType = GL_TRIANGLES);

	void Bake();

public:
	std::vector<Mesh> _meshes;
	std::vector<Triangle> _triangles;;
	//std::vector<int> _meshIndices;
	std::vector<std::string> _meshNames;
	PxTriangleMesh* _triangleMesh;

public:
	std::string _name;
	std::string _filename;
	BoundingBox _boundingBox;
};
