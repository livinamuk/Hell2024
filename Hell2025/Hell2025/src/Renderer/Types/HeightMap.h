#pragma once
#include <string>
#include "RendererCommon.h"
#include "../Physics/Physics.h"

struct HeightMap {

    unsigned int m_VAO = 0;
    unsigned int m_VBO = 0;
    unsigned int m_EBO = 0;
    int m_width = 0;
    int m_depth = 0;
    std::vector<Vertex> m_vertices;
    std::vector<unsigned int> m_indices;
    Transform m_transform;

    void Load(const std::string& filepath, float textureRepeat);
    void UploadToGPU();
    void CreatePhysicsObject();

    glm::vec3 GetWorldSpaceCenter();

    PxHeightField* m_pxHeightField = NULL;
    PxShape* m_pxShape = NULL;
    PxRigidStatic* m_pxRigidStatic = NULL;


};