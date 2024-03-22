#pragma once
#include <glm/vec3.hpp>
#include <vector>
#include "../../Common.h"
#include "../../Renderer/RendererCommon.h"

struct Wall {

    glm::vec3 begin = glm::vec3(0);
    glm::vec3 end;
    float height = 0;
    GLuint VAO = 0;
    GLuint VBO = 0;
 

    int materialIndex = 0;
    float wallHeight = 2.4f;
    Wall(glm::vec3 begin, glm::vec3 end, float height, int materialIndex);
    glm::vec3 GetNormal();
    glm::vec3 GetMidPoint();
    void Draw();
    std::vector<Transform> ceilingTrims;
    std::vector<Transform> floorTrims;
    std::vector<Line> collisionLines;


    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    RenderItem3D renderItem;
    int meshIndex = -1;

    void CreateVertexData();
    void CreateMeshGL();
    void UpdateRenderItem();
    RenderItem3D& GetRenderItem();
};