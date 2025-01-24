#include "VolumetricBloodSplatter.h"
#include "../API/OpenGL/GL_backEnd.h"
#include "../Core/AssetManager.h"

GLuint VolumetricBloodSplatter::s_vao;
GLuint VolumetricBloodSplatter::s_buffer_mode_matrices;
unsigned int VolumetricBloodSplatter::s_counter = 0;

VolumetricBloodSplatter::VolumetricBloodSplatter(glm::vec3 position, glm::vec3 rotation, glm::vec3 front) {
    m_transform.position = position;
    m_transform.rotation = rotation;
    m_transform.scale = glm::vec3(0.5f);
    m_front = front;

    if (s_counter == 0)
        m_type = 7;
    else if (s_counter == 1)
        m_type = 9;
    else if (s_counter == 2)
        m_type = 6;
    else if (s_counter == 3)
        m_type = 4;

    s_counter++;

    if (s_counter == 4)
        s_counter = 0;
}

void VolumetricBloodSplatter::Update(float deltaTime) {
    m_CurrentTime += deltaTime * 1.5f;
}

glm::mat4 VolumetricBloodSplatter::GetModelMatrix() {
    Transform bloodMeshTransform;
    bloodMeshTransform.position = m_transform.position;
    bloodMeshTransform.rotation = m_transform.rotation;

    Transform bloodMeshOffset;

    if (m_type == 0)
        bloodMeshOffset.position = glm::vec3(-0.08f, -0.23f, -0.155f);
    else if (m_type == 7)
        bloodMeshOffset.position = glm::vec3(-0.2300000042f, -0.5000000000f, -0.2249999940f);
    else if (m_type == 6)
        bloodMeshOffset.position = glm::vec3(-0.0839999989, -0.3799999952, -0.1500000060);
    else if (m_type == 8)
        bloodMeshOffset.position = glm::vec3(-0.1700000018f, -0.2290000021f, -0.1770000011f);
    else if (m_type == 9)
        bloodMeshOffset.position = glm::vec3(-0.0500000007, -0.2549999952, -0.1299999952);
    else if (m_type == 4)
        bloodMeshOffset.position = glm::vec3(-0.0500000045f, -0.4149999917f, -0.1900000125f);

    Transform rotTransform;
    rotTransform.rotation = glm::vec3(0, -HELL_PI / 2, 0);

    Transform scaleTransform;
    scaleTransform.scale = glm::vec3(3);

    return bloodMeshTransform.to_mat4() * scaleTransform.to_mat4() * rotTransform.to_mat4() * bloodMeshOffset.to_mat4();
}