#pragma once
#include "../../Common.h"
#include "../../Renderer/RendererCommon.h"

namespace OpenGLBackEnd {

	void InitMinimum();
    void UploadVertexData(std::vector<Vertex>& vertices, std::vector<unsigned int>& indices);
    void UploadWeightedVertexData(std::vector<WeightedVertex>& vertices, std::vector<uint32_t>& indices);
    void AllocateSkinnedVertexBufferSpace(int vertexCount);
    GLuint GetVertexDataVAO();
    GLuint GetVertexDataVBO();
    GLuint GetVertexDataEBO();
    GLuint GetWeightedVertexDataVAO();
    GLuint GetWeightedVertexDataVBO();
    GLuint GetWeightedVertexDataEBO();
    GLuint GetSkinnedVertexDataVAO();
    GLuint GetSkinnedVertexDataVBO();

    // Point cloud
    void CreatePointCloudVertexBuffer(std::vector<CloudPoint>& pointCloud);
    GLuint GetPointCloudVAO();
    GLuint GetPointCloudVBO();
}