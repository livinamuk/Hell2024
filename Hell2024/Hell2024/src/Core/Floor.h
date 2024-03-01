#pragma once
#include "../Common.h"

struct Floor {

    float x1, z1, x2, z2, height; // remove these. you are only using them to create the bathroom ceiling. 
    Vertex v1, v2, v3, v4;
    GLuint VAO = 0;
    GLuint VBO = 0;
    std::vector<Vertex> vertices;
    float textureScale = 1.0f;
    int materialIndex = 0;

    Floor(float x1, float z1, float x2, float z2, float height, int materialIndex, float textureScale);
    Floor(glm::vec3 pos1, glm::vec3 pos2, glm::vec3 pos3, glm::vec3 pos4, int materialIndex, float textureScale);
    void Draw();
    void CreateMesh();
    bool PointIsAboveThisFloor(glm::vec3 point);
};