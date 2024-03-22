#pragma once
#include <glm/vec3.hpp>
#include <vector>
#include "../../Common.h"
#include "../../Renderer/RendererCommon.h"

struct Floor {

    float x1, z1, x2, z2, height; // remove these. you are only using them to create the bathroom ceiling. 
    Vertex v1, v2, v3, v4;
    GLuint VAO = 0;
    GLuint VBO = 0;
    float textureScale = 1.0f;
    int materialIndex = 0;

    Floor(float x1, float z1, float x2, float z2, float height, int materialIndex, float textureScale);
    Floor(glm::vec3 pos1, glm::vec3 pos2, glm::vec3 pos3, glm::vec3 pos4, int materialIndex, float textureScale);
    void Draw();
    void CreateMeshGL();
    bool PointIsAboveThisFloor(glm::vec3 point);

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    RenderItem3D renderItem;
    int meshIndex = -1;

    void CreateVertexData();
    void UpdateRenderItem();
    RenderItem3D& GetRenderItem();
};