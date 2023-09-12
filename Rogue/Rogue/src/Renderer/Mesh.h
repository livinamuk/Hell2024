#pragma once
#include "../Common.h"
#include <vector>

class Mesh {

	public:
		Mesh();
		Mesh (std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::string name);
		void Draw();
		int GetIndexCount();

	private:
		unsigned int _VBO = 0;
		unsigned int _VAO = 0;
		unsigned int _EBO = 0;
		unsigned int _indexCount = 0;
		std::string _name;
};