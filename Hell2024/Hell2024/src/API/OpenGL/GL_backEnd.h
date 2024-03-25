#pragma once
#include "../../Common.h"

namespace OpenGLBackEnd {

	void InitMinimum();
    void UploadVertexData(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices);
    GLuint GetVertexDataVAO();
}