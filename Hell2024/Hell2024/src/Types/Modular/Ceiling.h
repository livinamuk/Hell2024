#pragma once
#include <glm/vec3.hpp>
#include <vector>
#include "../../Common.h"
#include "../../Renderer/RendererCommon.h"

struct Ceiling {
    float x1, z1, x2, z2, height;
    GLuint VAO = 0;
    GLuint VBO = 0;
    int materialIndex = 0;
    Ceiling(float x1, float z1, float x2, float z2, float height, int materialIndex);
    void Draw();

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    RenderItem3D renderItem;
    int meshIndex = -1;

    void CreateMeshGL();

    void CreateVertexData();
    void UpdateRenderItem();
    RenderItem3D& GetRenderItem();
};