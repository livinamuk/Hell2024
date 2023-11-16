#pragma once
#include "../Common.h"
#include "GameObject.h"
#include "AnimatedGameObject.h"

#define WALL_HEIGHT 2.4f

inline glm::vec3 NormalFromThreePoints(glm::vec3 pos0, glm::vec3 pos1, glm::vec3 pos2) {
    return glm::normalize(glm::cross(pos1 - pos0, pos2 - pos0));
}

inline void SetNormalsAndTangentsFromVertices(Vertex* vert0, Vertex* vert1, Vertex* vert2) {
    // Shortcuts for UVs
    glm::vec3& v0 = vert0->position;
    glm::vec3& v1 = vert1->position;
    glm::vec3& v2 = vert2->position;
    glm::vec2& uv0 = vert0->uv;
    glm::vec2& uv1 = vert1->uv;
    glm::vec2& uv2 = vert2->uv;
    // Edges of the triangle : postion delta. UV delta
    glm::vec3 deltaPos1 = v1 - v0;
    glm::vec3 deltaPos2 = v2 - v0;
    glm::vec2 deltaUV1 = uv1 - uv0;
    glm::vec2 deltaUV2 = uv2 - uv0;
    float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
    glm::vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
    glm::vec3 bitangent = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r;
    glm::vec3 normal = NormalFromThreePoints(vert0->position, vert1->position, vert2->position);
    vert0->normal = normal;
    vert1->normal = normal;
    vert2->normal = normal;
    vert0->tangent = tangent;
    vert1->tangent = tangent;
    vert2->tangent = tangent;
    vert0->bitangent = bitangent;
    vert1->bitangent = bitangent;
    vert2->bitangent = bitangent;
}

#define PROPOGATION_SPACING 1
#define PROPOGATION_WIDTH (MAP_WIDTH / PROPOGATION_SPACING)
#define PROPOGATION_HEIGHT (MAP_HEIGHT / PROPOGATION_SPACING)
#define PROPOGATION_DEPTH (MAP_DEPTH / PROPOGATION_SPACING)

struct CloudPoint {
    glm::vec4 position = glm::vec4(0);
    glm::vec4 normal = glm::vec4(0);
    glm::vec4 directLighting = glm::vec4(0);
};

struct Wall {
    glm::vec3 begin;
    glm::vec3 end;
    float height;
    bool hasTopTrim;
    bool hasBottomTrim;
    float topTrimBottom;
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint EBO = 0;
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices = { 0, 1, 2, 1, 0, 3 };
    int materialIndex = 0;

    float wallHeight = 2.4f;

    Wall(glm::vec3 begin, glm::vec3 end, float height, int materialIndex, bool hasTopTrim = false, bool hasBottomTrim = false) {
        this->materialIndex = materialIndex;
        this->begin = begin;
        this->end = end;
        this->height = height;
        this->hasTopTrim = hasTopTrim;
        this->hasBottomTrim = hasBottomTrim;
        Vertex v1, v2, v3, v4;
        v1.position = begin;
        v2.position = end + glm::vec3(0, height, 0);
        v3.position = begin + glm::vec3(0, height, 0);
        v4.position = end;
        this->topTrimBottom = v3.position.y - WALL_HEIGHT;
        glm::vec3 normal = NormalFromThreePoints(v1.position, v2.position, v3.position);
        v1.normal = normal;
        v2.normal = normal;
        v3.normal = normal;
        v4.normal = normal;

        float wallWidth = glm::distance(begin, end);
        float uv_x_low = 0;
        float uv_x_high = wallWidth / WALL_HEIGHT;
        float uv_y_low = begin.y / WALL_HEIGHT;
        float uv_y_high = (begin.y + height) / WALL_HEIGHT;
        float offsetY = 0.05f;

        uv_x_high *= 2;
        uv_y_high *= 2;

        uv_y_low -= offsetY;
        uv_y_high -= offsetY;
        v1.uv = glm::vec2(uv_x_low, uv_y_low);
        v2.uv = glm::vec2(uv_x_high, uv_y_high);
        v3.uv = glm::vec2(uv_x_low, uv_y_high);
        v4.uv = glm::vec2(uv_x_high, uv_y_low);
        SetNormalsAndTangentsFromVertices(&v1, &v2, &v3);
        SetNormalsAndTangentsFromVertices(&v2, &v1, &v4);
        vertices.push_back(v1);
        vertices.push_back(v2);
        vertices.push_back(v3);
        vertices.push_back(v4);
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, bitangent));
    }

    void Draw() {
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, 0);
    }
};

struct Floor {
    float x1, z1, x2, z2, height;
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint EBO = 0;
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices = { 0, 1, 2, 2, 3, 0 };
    int materialIndex = 0;

    Floor(float x1, float z1, float x2, float z2, float height, int materialIndex, float texScale = 1.0f) {
        this->materialIndex = materialIndex;
        this->x1 = x1;
        this->z1 = z1;
        this->x2 = x2;
        this->z2 = z2;
        this->height = height;
        Vertex v1, v2, v3, v4;
        v1.position = glm::vec3(x1, height, z1);
        v2.position = glm::vec3(x1, height, z2);
        v3.position = glm::vec3(x2, height, z2);
        v4.position = glm::vec3(x2, height, z1);
        glm::vec3 normal = NormalFromThreePoints(v1.position, v2.position, v3.position);
        v1.normal = normal;
        v2.normal = normal;
        v3.normal = normal;
        v4.normal = normal;
        float uv_x_low = x1 / texScale;
        float uv_x_high = x2 / texScale;
        float uv_y_low = z1 / texScale;
        float uv_y_high = z2 / texScale;
        v1.uv = glm::vec2(uv_x_low, uv_y_low);
        v2.uv = glm::vec2(uv_x_low, uv_y_high);
        v3.uv = glm::vec2(uv_x_high, uv_y_high);
        v4.uv = glm::vec2(uv_x_high, uv_y_low);
        SetNormalsAndTangentsFromVertices(&v1, &v2, &v3);
        SetNormalsAndTangentsFromVertices(&v3, &v4, &v1);
        vertices.push_back(v1);
        vertices.push_back(v2);
        vertices.push_back(v3);
        vertices.push_back(v4);
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, bitangent));
    }

    void Draw() {
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, 0);
    }
};

struct Ceiling {
    float x1, z1, x2, z2, height;
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint EBO = 0;
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices = { 2, 1, 0, 0, 3, 2 };
    int materialIndex = 0;

    Ceiling(float x1, float z1, float x2, float z2, float height, int materialIndex) {
        this->materialIndex = materialIndex;
        this->x1 = x1;
        this->z1 = z1;
        this->x2 = x2;
        this->z2 = z2;
        this->height = height;
        Vertex v1, v2, v3, v4;
        v1.position = glm::vec3(x1, height, z1);
        v2.position = glm::vec3(x1, height, z2);
        v3.position = glm::vec3(x2, height, z2);
        v4.position = glm::vec3(x2, height, z1);
        glm::vec3 normal = NormalFromThreePoints(v3.position, v2.position, v1.position);
        v1.normal = normal;
        v2.normal = normal;
        v3.normal = normal;
        v4.normal = normal;
        float scale = 2.0f;
        float uv_x_low = z1 / scale;
        float uv_x_high = z2 / scale;
        float uv_y_low = x1 / scale;
        float uv_y_high = x2 / scale;
        uv_x_low = x1;
        uv_x_high = x2;
        uv_y_low = z1;
        uv_y_high = z2;
        v1.uv = glm::vec2(uv_x_low, uv_y_low);
        v2.uv = glm::vec2(uv_x_low, uv_y_high);
        v3.uv = glm::vec2(uv_x_high, uv_y_high);
        v4.uv = glm::vec2(uv_x_high, uv_y_low);
        SetNormalsAndTangentsFromVertices(&v3, &v2, &v1);
        SetNormalsAndTangentsFromVertices(&v1, &v4, &v3);
        vertices.push_back(v1);
        vertices.push_back(v2);
        vertices.push_back(v3);
        vertices.push_back(v4);
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, bitangent));
    }
    void Draw() {
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, 0);
    }
};


namespace Scene {

    inline std::vector<Wall> _walls;
    inline std::vector<Floor> _floors;
    inline std::vector<Ceiling> _ceilings;
    inline std::vector<CloudPoint> _cloudPoints;
    inline std::vector<Triangle> _triangleWorld;
    inline std::vector<Line> _worldLines;
    inline std::vector<GameObject> _gameObjects;
    inline std::vector<AnimatedGameObject> _animatedGameObjects;
    inline std::vector<Light> _lights;
    //inline GridProbe _propogrationGrid[PROPOGATION_WIDTH][PROPOGATION_HEIGHT][PROPOGATION_DEPTH];

    void Init();
    void Update(float deltaTime);
    void LoadLightSetup(int index);
    GameObject* GetGameObjectByName(std::string);
    AnimatedGameObject* GetAnimatedGameObjectByName(std::string);
    std::vector<AnimatedGameObject>& GetAnimatedGameObjects();
    void CreatePointCloud();
}