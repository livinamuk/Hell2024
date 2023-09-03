#pragma once
#include "../Common.h"
#include <vector>

class Mesh {

	public:
		Mesh();
		Mesh (std::vector<Vertex> vertices, std::vector<unsigned int> indices);
		void Draw();

	private:
		unsigned int _VBO = 0;
		unsigned int _VAO = 0;
		unsigned int _EBO = 0;
		unsigned int _indexCount = 0;
};