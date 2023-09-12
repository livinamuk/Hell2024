#pragma once
#include "../Common.h"
#include "Mesh.h"

struct BoundingBox {
	glm::vec3 scale;
	glm::vec3 offsetFromModelOrigin;
};

class Model
{
public:
	void Load(std::string filepath);
	void Draw();
	//void Draw(Shader* shader, glm::mat4 modelMatrix, int primitiveType = GL_TRIANGLES);
	
public:
	std::vector<Mesh> _meshes; 
	std::vector<Triangle> _triangles;;

public:
	std::string _name;
	std::string _filename;
	BoundingBox _boundingBox;
};
