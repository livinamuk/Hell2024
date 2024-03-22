#pragma once
#include "../../Common.h"
#include "../../Types/Mesh.h"

namespace OpenGLBackEnd {

	void InitMinimum();
    //void UploadMesh(Mesh& mesh);
    void UploadVertexData(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices);
    GLuint GetVertexDataVAO();
}